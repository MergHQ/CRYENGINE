using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using CryEngine.Common;
using CryEngine.Common.BehaviorTree;

namespace CryEngine
{
	public class GenericNodeCreator : IManagedNodeCreator
	{
		#region Fields
		private static List<Node> s_nodes = new List<Node>();
		private Type _nodeType;
		#endregion

		#region Properties
		public Type NodeType { get { return _nodeType; } }
		#endregion

		#region Constructors
		public GenericNodeCreator(Type nodeType)
		{
			_nodeType = nodeType;
		}
		#endregion

		#region Methods
		public override Node Create()
		{
			Node node = (Node)Activator.CreateInstance(_nodeType);
			s_nodes.Add(node); // keep a reference to the node to prevent it from being garbage collected.
			return node;
		} 

		
		#endregion
	}

	public class BehaviorTreeNodeFactory
	{
		private static Dictionary<string, GenericNodeCreator> s_registry = new Dictionary<string, GenericNodeCreator>();
		
		public static void TryRegister(Type type)
		{
			if(!typeof(BehaviorTreeNodeBase).IsAssignableFrom(type) || type.IsAbstract)
			{
				return;
			}

			string name = type.Name;
			BehaviorTreeNodeAttribute behaviorNodeAttribute = (BehaviorTreeNodeAttribute)type.GetCustomAttributes(typeof(BehaviorTreeNodeAttribute), true).FirstOrDefault();
			if(behaviorNodeAttribute != null)
			{
				if(!behaviorNodeAttribute.Register)
				{
					return;
				}
				else if(!String.IsNullOrEmpty(behaviorNodeAttribute.Name))
				{
					name = behaviorNodeAttribute.Name;
				}
			}

			if (s_registry.ContainsKey(name))
			{
				return;
			}
			if(s_registry.Values.Any(x => x.NodeType == type))
			{
				return;
			}

			GenericNodeCreator creator = new GenericNodeCreator(type);
			s_registry.Add(name, creator);
			Global.gEnv.pMonoRuntime.RegisterManagedNodeCreator(name, creator);
		}
	}
}