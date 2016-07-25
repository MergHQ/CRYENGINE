using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace CryLinkHelper
{
	public struct XmlRPCValue
	{
		[XmlElement(ElementName = "i8", Type = typeof(Int64))]
		[XmlElement(ElementName = "string", Type = typeof(String))]
		[XmlElement(ElementName = "boolean", Type = typeof(bool))]
		public Object innerValue;
	}

	public struct XmlRPCParam
	{
		[XmlElement(ElementName = "value")]
		public XmlRPCValue value;
	}

	public class XmlRPCResponse
	{
		[XmlRoot(ElementName = "methodResponse")]
		public struct XmlResponse
		{
			[XmlArray(ElementName = "params")]
			[XmlArrayItem(ElementName = "param")]
			public XmlRPCParam[] paramsArray;
		}

		/// <summary>
		/// Creates a new instance from a stream.
		/// </summary>
		/// <param name="responseStream">A response StreamReader.</param>
		public XmlRPCResponse(StreamReader responseStream)
		{
			XmlSerializer serializer = new XmlSerializer(typeof(XmlResponse));
			m_response = (XmlResponse)serializer.Deserialize(responseStream);
		}

		/// <summary>
		/// Tries to cast the result to the expected type and returns it's value
		/// </summary>
		/// <typeparam name="T">Expected result type</typeparam>
		/// <returns>The result.</returns>
		/// <remarks>Throws an exception if the value doesn't match the expected type.</remarks>
		public T GetResultAs<T>()
		{
			if (m_response.paramsArray.Length == 1)
			{
				Object value = m_response.paramsArray[0].value.innerValue;
				if (value.GetType() == typeof(T))
				{
					return (T)value;
				}

				throw new Exception(String.Format("Wrong result type {0} expected was {1}", value.GetType(), typeof(T)));
			}

			throw new Exception(String.Format("Result has unexpected amount of return params '{0}'.", m_response.paramsArray.Length));
		}

		private XmlResponse m_response;
	}

	public class XmlRPCMethod
	{
		/*[XmlRoot(ElementName = "methodCall")]
		public struct XmlMethodCall
		{
			[XmlElement(ElementName = "methodName")]
			public String methodName;

			[XmlElement(ElementName = "params")]
			[XmlArrayItem(ElementName = "param")]
			public XmlRPCParam[] paramsArray;
		}*/

		/// <summary>
		/// Gets the method name.
		/// </summary>
		public String MethodName 
		{ 
			get; private set; 
		}

		/// <summary>
		/// Gets the connection string.
		/// </summary>
		public String Connection 
		{ 
			get; private set; 
		}

		/// <summary>
		/// Gets or sets whether the connection should be kept alive. Default is true.
		/// </summary>
		public bool KeepAlive 
		{
			get; set;
		}

		/// <summary>
		/// Creates a new instance that will call the specified method.
		/// </summary>
		/// <param name="connection">Connection string where to call the method.</param>
		/// <param name="methodName">Name of the method to call.</param>
		public XmlRPCMethod(String connection, String methodName)
		{
			MethodName = methodName;
			Connection = connection;
			KeepAlive = true;
		}

		/// <summary>
		/// Calls a method with given parameters.
		/// </summary>
		/// <typeparam name="T">Expected return value type.</typeparam>
		/// <param name="parameters">List of parameters.</param>
		/// <returns>The methods return value.</returns>
		public T Call<T>(ArrayList parameters)
		{
			String methodParams = "";
			if (parameters != null && parameters.Count > 0)
			{
				foreach (Object param in parameters)
				{
					methodParams += String.Format("<param><value><string>{0}</string></value></param>", param);
				}
			}

			String command = "<?xml version=\"1.0\"?><methodCall><methodName>" + MethodName + "</methodName><params>" + methodParams + "</params></methodCall>";
			byte[] bytes = Encoding.ASCII.GetBytes(command);

			System.Net.HttpWebRequest webRequest = (System.Net.HttpWebRequest)System.Net.WebRequest.Create(Connection);
			webRequest.Method = "POST";
			webRequest.ContentLength = bytes.Length;
			webRequest.KeepAlive = KeepAlive;

			using (System.IO.Stream stream = webRequest.GetRequestStream())
			{
				stream.Write(bytes, 0, bytes.Length);
			}

			try
			{
				XmlRPCResponse response = new XmlRPCResponse(new StreamReader(webRequest.GetResponse().GetResponseStream()));
				return response.GetResultAs<T>();
			}
			catch (Exception ex)
			{
				String error = String.Format("Call Failed! Full command: {0} \r\n\r\nInner Exception: {0}", command, ex.Message, ex);
				Console.Write(error);

				throw new Exception(error);
			}
		}
	}
}
