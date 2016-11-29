using System;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace CryEngine.NativeInternals
{
    internal static class Entity
    {
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public static void RegisterComponent(Type type, UInt64 guidHipart, UInt64 guidLopart);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public static void RegisterEntityWithDefaultComponent(string name, string category, string helper, string icon, bool hide, Type type);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public static object GetComponent(IntPtr entityPtr, Type type);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public static object GetOrCreateComponent(IntPtr entityPtr, Type type);

        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        extern public static void RegisterComponentProperty(Type type, PropertyInfo propertyInfo, string name, string label, string description, EntityPropertyType propertyType);
    }
}