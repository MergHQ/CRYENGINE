// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace CryEngine.Serialization
{
	internal class ObjectReader
	{
		internal ObjectReader(Stream stream)
		{
			stream.Seek(0, SeekOrigin.Begin);

			Reader = new BinaryReader(stream);
			Converter = new FormatterConverter();
			PopulateGuidLookup();
		}

		private BinaryReader Reader { get; set; }
		private FormatterConverter Converter { get; set; }

		private Dictionary<Type, Dictionary<int, object>> _typeObjects = new Dictionary<Type, Dictionary<int, object>>();
		private Dictionary<string, Type> _guidLookup = new Dictionary<string, Type>();

		private Type _deserializationCallbackType = typeof(IDeserializationCallback);
		private MethodInfo _deserializationMethod = typeof(IDeserializationCallback).GetMethod(nameof(IDeserializationCallback.OnDeserialization), BindingFlags.Instance | BindingFlags.Public | BindingFlags.DeclaredOnly);
		private Type _entityComponentType = typeof(EntityComponent);

		private void PopulateGuidLookup()
		{
			var assemblies = AppDomain.CurrentDomain.GetAssemblies();
			foreach(var assembly in assemblies)
			{
				var types = assembly.GetTypes();
				foreach(var type in types)
				{
					if(!type.IsClass)
					{
						continue;
					}

					var guid = ObjectWriter.GetGuid(type);
					if(guid != null)
					{
						_guidLookup.Add(guid, type);
					}
				}
			}
		}

		internal object Read()
		{
			return ReadInternal();
		}

		private object ReadInternal(object existingObject = null, bool isConstructed = false)
		{
			var objectType = (SerializedObjectType)Reader.ReadByte();

			switch(objectType)
			{
				case SerializedObjectType.Null:
					return null;
				case SerializedObjectType.Boolean:
					return Reader.ReadBoolean();
				case SerializedObjectType.Char:
					return Reader.ReadChar();
				case SerializedObjectType.SByte:
					return Reader.ReadSByte();
				case SerializedObjectType.Byte:
					return Reader.ReadByte();
				case SerializedObjectType.Int16:
					return Reader.ReadInt16();
				case SerializedObjectType.UInt16:
					return Reader.ReadUInt16();
				case SerializedObjectType.Int32:
					return Reader.ReadInt32();
				case SerializedObjectType.UInt32:
					return Reader.ReadUInt32();
				case SerializedObjectType.Int64:
					return Reader.ReadInt64();
				case SerializedObjectType.UInt64:
					return Reader.ReadUInt64();
				case SerializedObjectType.Single:
					return Reader.ReadSingle();
				case SerializedObjectType.Double:
					return Reader.ReadDouble();
				case SerializedObjectType.Decimal:
					return Reader.ReadDecimal();
				case SerializedObjectType.IntPtr:
					return new IntPtr(Reader.ReadInt64());
				case SerializedObjectType.UIntPtr:
					return new UIntPtr(Reader.ReadUInt64());
				case SerializedObjectType.Enum:
					return ReadEnum();
				case SerializedObjectType.Reference:
					return ReadReference();
				case SerializedObjectType.Object:
					return ReadObject(existingObject, isConstructed);
				case SerializedObjectType.Array:
					return ReadArray();
				case SerializedObjectType.Assembly:
					return ReadAssembly();
				case SerializedObjectType.Type:
					return ReadTypeWithReference();
				case SerializedObjectType.String:
					return ReadString();
				case SerializedObjectType.MemberInfo:
					return ReadMemberInfo();
				case SerializedObjectType.ISerializable:
					return ReadISerializable();
				case SerializedObjectType.EntityComponent:
					return ReadEntityComponent();
			}

			throw new ArgumentException("Tried to deserialize unknown object type!");
		}

		internal void ReadStatics()
		{
			var numTypes = Reader.ReadInt32();

			var fieldFlags = BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly;

			for(var i = 0; i < numTypes; i++)
			{
				if(!Reader.ReadBoolean())
				{
					continue;
				}

				var type = ReadType();

				ReadObjectBaseTypeMembers(null, type, fieldFlags, false);
			}
		}

		private object ReadEnum()
		{
			var type = ReadType();
			var value = Read();

			return Enum.ToObject(type, value);
		}

		private object ReadReference()
		{
			var referenceId = Reader.ReadInt32();
			var type = ReadType();

			Dictionary<int, object> dict;
			if(_typeObjects.TryGetValue(type, out dict))
			{
				object obj;
				if(dict.TryGetValue(referenceId, out obj))
				{
					return obj;
				}
			}
			return null;
		}

		private void AddReference(Type type, object obj, int referenceId)
		{
			Dictionary<int, object> objects;
			if(!_typeObjects.TryGetValue(type, out objects))
			{
				objects = new Dictionary<int, object>();
				_typeObjects.Add(type, objects);
			}

			objects.Add(referenceId, obj);
		}

		#region Reference types
		private object ReadEntityComponent()
		{
			var referenceId = Reader.ReadInt32();
			var componentTypeGuid = (EntityComponent.GUID)Read();

			var type = EntityComponent.GetComponentTypeByGUID(componentTypeGuid);

			var obj = type != null ? Activator.CreateInstance(type) : null;
			if(type != null)
			{
				AddReference(type, obj, referenceId);
			}

			ReadObjectMembers(obj, type, true);

			return obj;
		}

		private object ReadObject(object existingObject, bool isConstructed)
		{
			var referenceId = Reader.ReadInt32();
			var type = ReadType();

			if(existingObject == null && type != null)
			{
				existingObject = FormatterServices.GetUninitializedObject(type);
			}

			if(type != null)
			{
				AddReference(type, existingObject, referenceId);
			}

			ReadObjectMembers(existingObject, type, isConstructed);

			return existingObject;
		}

		private void ReadObjectMembers(object obj, Type type, bool isConstructed)
		{
			var numBaseTypes = Reader.ReadInt32();
			var fieldFlags = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.DeclaredOnly;

			ReadObjectBaseTypeMembers(obj, type, fieldFlags, isConstructed);

			for(var i = 0; i < numBaseTypes; i++)
			{
				var baseType = ReadType();

				ReadObjectBaseTypeMembers(obj, baseType, fieldFlags, isConstructed);
			}

			if(_deserializationCallbackType.IsAssignableFrom(type))
			{
				_deserializationMethod.Invoke(obj, new object[] { this });
			}
		}

		private void ReadObjectBaseTypeMembers(object obj, Type objectType, BindingFlags flags, bool setSerializedFieldsOnly)
		{
			var numFields = Reader.ReadInt32();
			var typeChanged = obj == null ? false : !objectType.IsAssignableFrom(obj.GetType());
			for(var iField = 0; iField < numFields; iField++)
			{
				var fieldName = Reader.ReadString();
				var fieldInfo = objectType?.GetField(fieldName, flags);

				var canWrite = !typeChanged // Writing a new type is not possible.
					&& fieldInfo != null // Can't write to non-existing field.
					&& !fieldInfo.IsLiteral // Can't write to const objects
					&& (obj == null) == fieldInfo.IsStatic; // If the object is null it can't be written, unless the field is static.

				var existingObject = canWrite ? fieldInfo.GetValue(obj) : null;
				var fieldValue = ReadInternal(existingObject, false);

				if(canWrite)
				{
					if(setSerializedFieldsOnly)
					{
						// Check if the field is a backing field for a property.
						if(!fieldInfo.IsDefined(objectType, false))
						{
							// Extract the property's name from the backing field's name.
							// Backing fields are in the format "<NameOfProperty>k_backingfield".
							int charIndex = fieldName.IndexOf('>');
							if(charIndex < 1)
							{
								continue;
							}
							var propertyName = fieldName.Substring(1, fieldName.IndexOf('>') - 1);
							var propertyInfo = objectType.GetProperty(propertyName, flags);
							if(propertyInfo == null)
							{
								continue;
							}

							var isEntityProperty = propertyInfo.GetCustomAttributes(typeof(EntityPropertyAttribute), false).Length != 0;

							// Always write to EntityComponents, otherwise the Entity property will be null.
							if(isEntityProperty || objectType == _entityComponentType)
							{
								fieldInfo.SetValue(obj, fieldValue);
							}
						}
					}
					else
					{
						fieldInfo.SetValue(obj, fieldValue);
					}
				}
			}

			var numEvents = Reader.ReadInt32();
			for(var iEvent = 0; iEvent < numEvents; iEvent++)
			{
				var eventName = Reader.ReadString();

				if(Reader.ReadBoolean())
				{
					var delegateType = ReadType();
					var functionCount = Reader.ReadInt32();

					EventInfo eventInfo = objectType != null ? objectType.GetEvent(eventName, flags) : null;

					for(int i = 0; i < functionCount; i++)
					{
						var methodInfo = Read() as MethodInfo;
						var target = Read();

						Delegate parsedDelegate;

						if(target != null)
						{
							parsedDelegate = Delegate.CreateDelegate(delegateType, target, methodInfo);
						}
						else
						{
							parsedDelegate = Delegate.CreateDelegate(delegateType, methodInfo);
						}

						if(eventInfo != null)
						{
							var addMethod = eventInfo.GetAddMethod(true);
							addMethod.Invoke(obj, new[] { parsedDelegate });
						}
					}
				}
			}

			if(objectType != null)
			{
				// Special case, always get native pointers for SWIG director types
				var swigCPtr = objectType.GetField("swigCPtr", flags);
				if(swigCPtr != null)
				{
					// Start with director handling, aka support for cross-language polymorphism
					var swigDirectorConnect = objectType.GetMethod("SwigDirectorConnect", flags);
					if(swigDirectorConnect != null)
					{
						var swigCMemOwn = objectType.GetField("swigCMemOwn", flags);

						// Should not be null, if it is we need to check if something is wrong
						var constructor = objectType.GetConstructor(Type.EmptyTypes);

						var newObj = constructor.Invoke(null);

						swigCMemOwn.SetValue(newObj, false);
						swigCMemOwn.SetValue(obj, true);

						var handleReference = new HandleRef(obj, ((HandleRef)swigCPtr.GetValue(newObj)).Handle);

						swigCPtr.SetValue(obj, handleReference);
						swigDirectorConnect.Invoke(obj, null);
					}
					else // Support for other types, such as Vec3 (where we should simply take ownership of the native pointer
					{
						var handleReference = new HandleRef(obj, ((HandleRef)swigCPtr.GetValue(obj)).Handle);
						swigCPtr.SetValue(obj, handleReference);
					}
				}
			}
		}

		private object ReadString()
		{
			var referenceId = Reader.ReadInt32();
			var stringValue = Reader.ReadString();

			AddReference(typeof(string), stringValue, referenceId);

			return stringValue;
		}

		private object ReadArray()
		{
			var referenceId = Reader.ReadInt32();

			var arrayLength = Reader.ReadInt32();
			var elementType = ReadType();

			var array = Array.CreateInstance(elementType, arrayLength);

			AddReference(array.GetType(), array, referenceId);

			for(var i = 0; i < arrayLength; i++)
			{
				array.SetValue(Read(), i);
			}

			return array;
		}

		private object ReadMemberInfo()
		{
			var referenceId = Reader.ReadInt32();

			var memberName = Reader.ReadString();

			var declaringType = ReadType();
			var memberType = (MemberTypes)Reader.ReadByte();

			var isStatic = Reader.ReadBoolean();
			var bindingFlags = BindingFlags.DeclaredOnly | BindingFlags.Public | BindingFlags.NonPublic;
			if(isStatic)
			{
				bindingFlags |= BindingFlags.Static;
			}
			else
			{
				bindingFlags |= BindingFlags.Instance;
			}

			MemberInfo memberInfo = null;

			switch(memberType)
			{
				case MemberTypes.Method:
				{
					var parameterCount = Reader.ReadInt32();
					var parameters = new Type[parameterCount];

					for(int i = 0; i < parameterCount; i++)
						parameters[i] = ReadType();

					memberInfo = declaringType.GetMethod(memberName, bindingFlags, null, parameters, null);
					break;
				}
				case MemberTypes.Field:
				{
					memberInfo = declaringType.GetField(memberName, bindingFlags);
					break;
				}
				case MemberTypes.Property:
				{
					memberInfo = declaringType.GetProperty(memberName, bindingFlags);
					break;
				}
			}

			AddReference(memberInfo.GetType(), memberInfo, referenceId);

			return memberInfo;
		}

		private object ReadISerializable()
		{
			var referenceId = Reader.ReadInt32();
			var type = ReadType();
			var memberCount = Reader.ReadInt32();

			var serInfo = new SerializationInfo(type, Converter);

			for(var i = 0; i < memberCount; i++)
			{
				var memberName = Reader.ReadString();
				var memberType = ReadType();
				var memberValue = Read();

				if(memberType != null)
				{
					serInfo.AddValue(memberName, memberValue, memberType);
				}
			}

			var constructor = type.GetConstructor(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance, null, new Type[] { typeof(SerializationInfo), typeof(StreamingContext) }, null);
			var obj = constructor.Invoke(new object[] { serInfo, new StreamingContext(StreamingContextStates.CrossAppDomain) });

			if(_deserializationCallbackType.IsAssignableFrom(type))
			{
				_deserializationMethod.Invoke(obj, new object[] { this });
			}

			AddReference(type, obj, referenceId);

			return obj;
		}

		private Type ReadTypeWithReference()
		{
			var referenceId = Reader.ReadInt32();
			var type = ReadType();

#pragma warning disable RECS0035 // Possible mistaken call to 'object.GetType()'
			if(type != null)
			{
				AddReference(type.GetType(), type, referenceId);
			}
#pragma warning restore RECS0035 // Possible mistaken call to 'object.GetType()'

			return type;
		}

		private Type ReadType()
		{
			Type type = null;
			if(Reader.ReadBoolean())
			{
				var elementType = ReadType();
				return elementType.MakeArrayType();
			}
			else if(Reader.ReadBoolean())
			{
				string guid = Reader.ReadString();
				_guidLookup.TryGetValue(guid, out type);
			}

			var isGeneric = Reader.ReadBoolean();

			// Only set the type if it isn't set yet.
			// The GUID lookup type is prioritized.
			if(type == null)
			{
				string typeName = Reader.ReadString();
				type = GetType(typeName);
			}
			else
			{
				GetType(Reader.ReadString());
			}

			if(isGeneric)
			{
				var numGenericArgs = Reader.ReadInt32();
				var genericArgs = new Type[numGenericArgs];
				for(int i = 0; i < numGenericArgs; i++)
				{
					Type genericType = ReadType();
					genericArgs[i] = genericType;
				}

				if(type != null)
				{
					type = type.MakeGenericType(genericArgs);
				}
			}

			return type;
		}

		private Type GetType(string typeName)
		{
			if(typeName == null)
			{
				throw new ArgumentNullException(nameof(typeName));
			}

			if(typeName.Length == 0)
			{
				throw new ArgumentException(string.Format("{0} cannot have zero length!", nameof(typeName)));
			}

			var type = Type.GetType(typeName);
			if(type != null)
				return type;

			foreach(var assembly in AppDomain.CurrentDomain.GetAssemblies())
			{
				type = assembly.GetType(typeName);
				if(type != null)
					return type;
			}

			return null;
		}

		private Assembly ReadAssembly()
		{
			var referenceId = Reader.ReadInt32();
			var assembly = Assembly.LoadFrom(Reader.ReadString());
			if(assembly != null)
			{
				AddReference(assembly.GetType(), assembly, referenceId);
			}

			return assembly;
		}
		#endregion
	}
}
