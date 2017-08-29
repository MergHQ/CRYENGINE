using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Xml.Linq;
using System.Xml.Serialization;

namespace CryBackend
{
	public static class ConfigUtils
	{
		public const string ELEMENT_NAME_SHARED = "Shared";
		public const string ATTRIB_NAME_WITH = "With";
		public const string WILDCAST = "*";
		static public int ReadIntValue(XElement config, string name, int defaultValue = 0)
		{
			var attr = config.Attribute(name);
			if (attr == null)
				return defaultValue;

			return int.Parse(attr.Value);
		}

		static public float ReadFloatValue(XElement config, string name, float defaultValue = 0)
		{
			var attr = config.Attribute(name);
			if (attr == null)
				return defaultValue;

			return float.Parse(attr.Value);
		}

		static public string ReadStringValue(XElement config, string name, string defaultValue = "")
		{
			var attr = config.Attribute(name);
			if (attr == null)
				return defaultValue;

			return attr.Value;
		}

		// TODO: automatically locate a sub XML node, e.g. with Annotation on config types
		public static T FromXml<T>(string strXml)
			where T : class
		{
			using (var reader = new StringReader(strXml))
			{
				var ser = new XmlSerializer(typeof(T), NodeConfig.NS);
				return (T)ser.Deserialize(reader);
			}
		}

		public static T FromXml<T>(XElement xelem)
			where T : class
		{
			return FromXml<T>(xelem.ToString());
		}

		public static XElement Merge(XElement config, string role)
		{
			XElement roleConfig = null;
			try
			{
				roleConfig = config.GetElement(role);
			}
			finally
			{
				if (roleConfig == null)
				{
					roleConfig = new XElement(XName.Get(role, NodeConfig.NS));
					config.Add(roleConfig);
				}
			}

			foreach (var shared in config.GetElements(ELEMENT_NAME_SHARED))
			{
				var sharedWith = shared.Attribute(ATTRIB_NAME_WITH);
				if (sharedWith == null)
					continue;

				if (sharedWith.Value == WILDCAST || sharedWith.Value.Split(',').Contains(role))
					roleConfig.Add(shared.Elements());
			}
			return config;
		}

		public static XElement MergeAll(XElement config)
		{
			foreach (var roleName in NodeRoleUtils.EnumRoleNames())
			{
				var roleTag = roleName.Replace("_", ".");
				config = Merge(config, roleTag);
			}
			return config;
		}

		public static XElement GenPerNodeConfig(XElement config, string nodeName)
		{
			var xmlns = NodeConfig.NS;

			var globalCfg = config.GetElement("Global", xmlns);

			var combined = new XElement(XName.Get("NodeConfig", xmlns));
			combined.Add(globalCfg.Elements());

			if (string.IsNullOrEmpty(nodeName))
				return combined;

			combined.SetAttributeValue("Name", nodeName);
			if (!config.Elements().Any(elem => elem.Name == XName.Get("Node", xmlns) && elem.Attribute("Name").Value == nodeName))
				return combined;

			var nodeCfg = config.Elements().First(elem => elem.Name == XName.Get("Node", xmlns) && elem.Attribute("Name").Value == nodeName);
			foreach (string role in Enum.GetNames(typeof(NodeRole)))
			{
				XName roleTag = XName.Get(role.Replace('_', '.'), xmlns);


				var existingOnes = (from e1 in combined.Elements(roleTag)
									from e2 in e1.Elements()
									select e2).ToArray();

				foreach (var nodeElem in nodeCfg.Elements(roleTag).SelectMany(e => e.Elements()))
				{
					var found = existingOnes.FirstOrDefault(e => e.Name == nodeElem.Name);
					if (found == null)
					{
						combined.GetOrAddElement(roleTag.LocalName, roleTag.NamespaceName).Add(nodeElem);
					}
					else if (found.Name == "{urn:orleans}OrleansConfiguration")
					{
						//TODO need to remove this specific hack later
						var overrideNode = new XElement(XName.Get("Override", found.Name.NamespaceName), new XAttribute("Node", nodeName));
						overrideNode.Add(nodeElem.Elements());
						found.Add(overrideNode);
					}
					else
					{
						found.ReplaceAll(Enumerable.Concat<object>(nodeElem.Nodes(), nodeElem.Attributes()).ToArray());
					}
				}
			}
			return combined;
		}

		public static void SetAddressIfNotExist(IEnumerable<XElement> configs)
		{
			foreach (var config in configs)
			{
				SetAddressIfNotExist(config);
			}
		}

		// Set 'Address' attribute with default IPv4 address if it's missing.
		// Otherwise leave it as it was.
		public static void SetAddressIfNotExist(XElement config)
		{
			if (config == null)
				return;

			var addressAttr = config.Attribute("Address");
			if (addressAttr == null)
			{
				var ipv4Addrs = NetUtils.GetIPv4Addresses().ToList();
				if (ipv4Addrs.Count == 1)
				{
					var address = ipv4Addrs[0].ToString();
					config.SetAttributeValue("Address", address);
				}
			}
		}

		public static IPEndPoint ReadEndPointConfig(XElement fepConfig)
		{
			var endpointElem = fepConfig.GetElement("Endpoint");

			if (endpointElem == null)
				throw new ApplicationException("Missing <Endpoint> Element");

			SetAddressIfNotExist(endpointElem);
			var portAttr = endpointElem.Attribute("Port");
			if (portAttr == null)
				throw new ApplicationException("Missing 'Port' attribute for Element <EndPoint>");

			var addrAttr = endpointElem.Attribute("Address");
			if (addrAttr == null)
				throw new ApplicationException("Missing 'Address' attribute for Element <Endpoint>");

			return NetUtils.GetIPEndPoint(addrAttr.Value, ushort.Parse(portAttr.Value));
		}

		public static string ReadAccessAddrConfig(XElement fepConfig)
		{
			var endpointElem = fepConfig.GetElement("Endpoint");

			if (endpointElem == null)
				throw new ApplicationException("Missing <Endpoint> Element");

			return endpointElem.Attribute("AccessAddr").Value;
		}
	}
}
