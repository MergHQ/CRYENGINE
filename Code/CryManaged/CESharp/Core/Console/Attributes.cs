// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.	

using System;
using System.Reflection;

using CryEngine;
using CryEngine.Common;

using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;

/// <summary>
/// All framework relevant Attributes.
/// </summary>
namespace CryEngine.Attributes
{
	[AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
	public sealed class ConsoleCommandAttribute : Attribute
	{
		public ConsoleCommandAttribute(string commandName, uint commandFlag, string commandHelp)
		{
			Name = commandName;
			Flags = commandFlag;
			Comments = commandHelp;
		}

		public string Name
		{
			get;
			private set;
		}

		public string Comments
		{
			get;
			private set;
		}

		public uint Flags
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// Thrown when Console Command Attributes are applied wrongly
	/// </summary>
	public sealed class ConsoleCommandConfigurationException :Exception
	{
		public ConsoleCommandConfigurationException(string msg):base(msg)
		{

		}
	}

	public sealed class ConsoleCommandAttributeManager
	{
		[Conditional("DEBUG")]
		private static void ValidateConsoleCommandRegisterAttributes(ref List<MethodInfo> processedlist, Assembly targetAssembly)
		{
			// detect attributes from all methods
			var totalAttributes = targetAssembly.GetTypes().SelectMany(x => x.GetMethods(BindingFlags.Static | BindingFlags.Instance | BindingFlags.NonPublic | BindingFlags.Public))
				.Where(y => y.GetCustomAttributes().OfType<ConsoleCommandAttribute>().Any())
				.ToList();
						
			//filter attributes that are detected but not processed, these are configured wrongly !
			var filteredAttributes = totalAttributes.Except(processedlist).ToList();
			if(filteredAttributes.Count >0 )
			{
				StringBuilder msg = new StringBuilder();
				foreach (var filteredAttribute in filteredAttributes)
				{
					msg.Append("Attribute on method").Append(filteredAttribute.Name).Append(" is not processed").AppendLine();
				}
				throw new ConsoleCommandConfigurationException(msg.ToString());
			}
		}

		/// <summary>
		/// Parse the specified assembly for all public, static methods marked with attribute "ConsoleCommandRegisterAttribute". Ignores all methods otherwise.
		/// </summary>
		/// <param name="targetAssembly"></param>
		public static void RegisterAttribute(Assembly targetAssembly)
		{
			//do reflection to invoke the attribute for console command function
			Assembly assembly = targetAssembly == null ? Assembly.GetExecutingAssembly() : targetAssembly;

			var processedList = assembly.GetTypes().SelectMany(x => x.GetMethods(BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public))
				.Where(y => y.GetCustomAttributes().OfType<ConsoleCommandAttribute>().Any())
				.ToList();

			foreach(MethodInfo methodInfo in processedList)
			{
				CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate managedConsoleCmdDelegate = Delegate.CreateDelegate(typeof(CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate), methodInfo) as CryEngine.ConsoleCommand.ManagedConsoleCommandFunctionDelegate;
				if (managedConsoleCmdDelegate != null)
				{
					ConsoleCommandAttribute consoleAttribute = methodInfo.GetCustomAttribute<ConsoleCommandAttribute>() as ConsoleCommandAttribute;
					if(consoleAttribute != null)
					{
						CryEngine.ConsoleCommand.RegisterManagedConsoleCommandFunction(consoleAttribute.Name, consoleAttribute.Flags, consoleAttribute.Comments, managedConsoleCmdDelegate);
					}
				}
			}
			ValidateConsoleCommandRegisterAttributes(ref processedList, assembly);
		}
	}
	
	/// <summary>
	/// Responsible to registration, processing and validation of console variable attributes. Currently there are 4 types of console variable attributes.
	/// 1. String - Can be read and modified after declaration
	/// 2. Integer (32-bit) - Can be read modified after declaration
	/// 3. Integer (64-bit) - Can be read and modified after declaration
	/// 4. Float - Can be read and modified after declaration
	/// </summary>
	public class ConsoleVariableAttributeManager
	{
		public static void RegisterAttribute(Assembly targetAssembly)
		{
			RegisterSubAttribute<ConsoleVariableStringAttribute, ConsoleVariableAttributeStringProperty>(targetAssembly);
			RegisterSubAttribute<ConsoleVariableIntegerAttribute, ConsoleVariableAttributeIntegerProperty>(targetAssembly);
			RegisterSubAttribute<ConsoleVariableInteger64Attribute, ConsoleVariableAttributeInteger64Property>(targetAssembly);
			RegisterSubAttribute<ConsoleVariableFloatAttribute, ConsoleVariableAttributeFloatProperty>(targetAssembly);
		}
		private static void RegisterSubAttribute<TAttribute, TAttributeProperty>(Assembly targetAssembly) 
			where TAttribute: ConsoleVariableAttribute
			where TAttributeProperty: class,new()
		{
			Assembly assembly = targetAssembly == null ? Assembly.GetExecutingAssembly() : targetAssembly;
			var processedTypes = assembly.GetTypes().SelectMany(x => x.GetFields(BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public))
				.Where(y => y.GetCustomAttributes().OfType<TAttribute>().Any())
				.ToList();
			foreach (FieldInfo fieldInfo in processedTypes)
			{
				Type fieldType = fieldInfo.FieldType;
				
				TAttributeProperty attributeProperty = fieldInfo.GetValue(null) as TAttributeProperty;
				ConsoleVariableAttribute consoleVariableAttribute = fieldInfo.GetCustomAttribute<ConsoleVariableAttribute>() as ConsoleVariableAttribute;
				if (fieldType == typeof(TAttributeProperty))
				{
					if (attributeProperty != null)
					{
						ICVar newVar = null;
						ProcessConsoleVariableProperty(consoleVariableAttribute, attributeProperty, out newVar);
					}
					else 
					{
						throw new ConsoleVariableConfigurationException("Console Variable Attribute " + consoleVariableAttribute.Name+" cannot be applied to static variable " + fieldType.FullName + " which is null!");
					}
				}
				else // attribute declared does not match property value 
				{
					throw new ConsoleVariableConfigurationException("Attribute ["+consoleVariableAttribute.Name+"] type does not match Property type "+fieldType);
				}
			}
		}
		
		private static void ProcessConsoleVariableProperty(ConsoleVariableAttribute consoleVariableAttribute, object property, out ICVar icvar)
		{
			icvar = null;
			ConsoleVariableAttributeStringProperty stringProperty = property as ConsoleVariableAttributeStringProperty;
			if(stringProperty != null)
			{
				ConsoleVariableStringAttribute stringAttribute = consoleVariableAttribute as ConsoleVariableStringAttribute;
				string value = stringProperty.Content == null ? stringAttribute.Content : stringProperty.Content;
				bool success = ConsoleVariable.Register(consoleVariableAttribute.Name, value, consoleVariableAttribute.Flags, consoleVariableAttribute.Comments, stringProperty.OnValueChanged, out icvar);
				if(success)
				{
					stringProperty.SetInternalContent(value, icvar);
				}
				return;
			}
			ConsoleVariableAttributeIntegerProperty integerProperty = property as ConsoleVariableAttributeIntegerProperty;
			if(integerProperty != null)
			{
				ConsoleVariableIntegerAttribute integerAttribute = consoleVariableAttribute as ConsoleVariableIntegerAttribute;
				int value = integerAttribute.Content.HasValue ? integerAttribute.Content.Value : integerProperty.Content;
				bool success = ConsoleVariable.Register(consoleVariableAttribute.Name, value, consoleVariableAttribute.Flags, consoleVariableAttribute.Comments, integerProperty.OnValueChanged, out icvar);
				if(success)
				{
					integerProperty.SetInternalContent(value, icvar);
				}
				return;
			}

			ConsoleVariableAttributeFloatProperty floatProperty = property as ConsoleVariableAttributeFloatProperty;
			if (floatProperty != null)
			{
				ConsoleVariableFloatAttribute floatAttribute = consoleVariableAttribute as ConsoleVariableFloatAttribute;
				float value = floatAttribute.Content.HasValue ? floatAttribute.Content.Value : floatProperty.Content;
				bool success = ConsoleVariable.Register(consoleVariableAttribute.Name, value, consoleVariableAttribute.Flags, consoleVariableAttribute.Comments, floatProperty.OnValueChanged, out icvar);
				if (success)
				{
					floatProperty.SetInternalContent(value, icvar);
				}
				return;
			}

			ConsoleVariableAttributeInteger64Property integer64Property = property as ConsoleVariableAttributeInteger64Property;
			if(integer64Property != null)
			{
				ConsoleVariableInteger64Attribute integer64Attribute = consoleVariableAttribute as ConsoleVariableInteger64Attribute;
				long value = integer64Attribute.Content.HasValue ? integer64Attribute.Content.Value : integer64Property.Content;
				bool success = ConsoleVariable.Register(consoleVariableAttribute.Name, value, consoleVariableAttribute.Flags, consoleVariableAttribute.Comments, integer64Property.OnValueChanged, out icvar);
				if(success)
				{
					integer64Property.SetInternalContent(value, icvar);
				}
			}
		}
	}
	
	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableFloatAttribute : ConsoleVariableAttribute
	{
		public ConsoleVariableFloatAttribute(string commandName, float value, uint commandFlag, string commandHelp) : base(commandName, commandFlag, commandHelp)
		{
			Content = value;
		}

		public float? Content
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableInteger64Attribute : ConsoleVariableAttribute
	{
		public ConsoleVariableInteger64Attribute(string commandName, long value, uint commandFlag, string commandHelp) : base(commandName, commandFlag, commandHelp)
		{
			Content = value;
		}

		public long? Content
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableIntegerAttribute : ConsoleVariableAttribute
	{
		public ConsoleVariableIntegerAttribute(string commandName, int value, uint commandFlag, string commandHelp) : base(commandName, commandFlag, commandHelp)
		{
			Content = value;
		}

		public int? Content
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableStringAttribute : ConsoleVariableAttribute
	{
		public ConsoleVariableStringAttribute(string commandName, string value, uint commandFlag, string commandHelp) : base(commandName, commandFlag, commandHelp)
		{
			Content = value;
		}

		public string Content
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// The base class for Console Variable attributes. Cannot be used by user
	/// </summary>
	[AttributeUsage(AttributeTargets.Field, AllowMultiple = false, Inherited = true)]
	public abstract class ConsoleVariableAttribute : Attribute
	{
		public ConsoleVariableAttribute(string commandName, uint commandFlag, string commandHelp)
		{
			Name = commandName;
			Flags = commandFlag;
			Comments = commandHelp;
		}

		public string Name
		{
			get;
			private set;
		}

		public string Comments
		{
			get;
			private set;
		}

		public uint Flags
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableAttributeFloatProperty
	{
		public ConsoleVariableAttributeFloatProperty()
		{
		}

		public float Content
		{
			get
			{
				return m_value;
			}
			set
			{
				NotifyOnChanged(value);
			}
		}

		internal void OnValueChanged(ICVar var)
		{
			m_value = var.GetFVal();
		}

		internal void NotifyOnChanged(float newValue)
		{
			if (m_icVar == null) return;
			m_icVar.Set(newValue); 
		}

		public override string ToString()
		{
			return "ConsoleVariableAttributeFloatProperty[" + Content + "]";
		}

		// does not activate managed console variable listener. Called when console variable has been successfully registered natively
		internal void SetInternalContent(float newValue, ICVar icVar)
		{
			m_value = newValue;
			m_icVar = icVar;
		}

		private float m_value;
		private ICVar m_icVar;
	}


	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableAttributeInteger64Property
	{
		public ConsoleVariableAttributeInteger64Property()
		{
			m_valueInText = new StringBuilder(20); //reserve capacity to hold 64-bit integer
		}

		public long Content
		{
			get
			{
				return m_value;
			}
			set
			{
				NotifyOnChanged(value);
			}
		}

		internal void OnValueChanged(ICVar var)
		{
			m_value = var.GetI64Val();
		}

		internal void NotifyOnChanged(long newValue)
		{
			if (m_icVar == null) return;
			m_valueInText.Clear();
			m_valueInText.Append(newValue);
			m_icVar.Set(m_valueInText.ToString());
		}

		public override string ToString()
		{
			return "ConsoleVariableAttributeInteger64Property[" + Content + "]";
		}

		// does not activate managed console variable listener. Called when console variable has been successfully registered natively
		internal void SetInternalContent(long newValue, ICVar icVar)
		{
			m_value = newValue;
			m_icVar = icVar;
		}

		private long m_value;
		private ICVar m_icVar;
		private StringBuilder m_valueInText;
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableAttributeIntegerProperty
	{
		public ConsoleVariableAttributeIntegerProperty()
		{
		}

		public int Content
		{
			get
			{
				return m_value;
			}
			set
			{
				NotifyOnChanged(value);
			}
		}

		internal void OnValueChanged(ICVar var)
		{
			m_value = var.GetIVal();
		}

		internal void NotifyOnChanged(int newValue)
		{
			if (m_icVar == null) return;
			m_icVar.Set(newValue);
		}

		public override string ToString()
		{
			return "ConsoleVariableAttributeIntegerProperty[" + Content + "]";
		}

		// does not activate managed console variable listener. Called when console variable has been successfully registered natively
		internal void SetInternalContent(int newValue, ICVar icVar)
		{
			m_value = newValue;
			m_icVar = icVar;
		}

		private int m_value;
		private ICVar m_icVar;
	}

	/// <summary>
	/// 
	/// </summary>
	public class ConsoleVariableAttributeStringProperty
	{
		public ConsoleVariableAttributeStringProperty()
		{
		}

		public String Content
		{
			get
			{
				return m_value;
			}
			set
			{
				NotifyOnChanged(value);
			}
		}

		internal void OnValueChanged(ICVar var)
		{
			m_value = var.GetString();
		}

		internal void NotifyOnChanged(string newValue)
		{
			if (m_icVar == null) return;
			m_icVar.Set(newValue);
		}

		public override string ToString()
		{
			return "ConsoleVariableAttributeStringProperty[" + Content + "]";
		}

		// does not activate managed console variable listener. Called when console variable has been successfully registered natively
		internal void SetInternalContent(string newValue, ICVar icVar)
		{
			m_value = newValue;
			m_icVar = icVar;
		}

		private String m_value;
		private ICVar m_icVar;
	}

	/// <summary>
	/// Thrown when Console Variable Attributes are configured wrongly
	/// </summary>
	public sealed class ConsoleVariableConfigurationException : Exception
	{
		public ConsoleVariableConfigurationException(string msg) : base(msg)
		{
		}
	}
}