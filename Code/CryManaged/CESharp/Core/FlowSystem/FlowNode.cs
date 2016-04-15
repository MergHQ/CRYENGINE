// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using CryEngine.Common;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Linq;
using System.Reflection;

/// <summary>
/// All classes related to FlowNode usage.
/// </summary>
namespace CryEngine.FlowSystem
{
	/// <summary>
	/// Creates a FlowNode of Type T.
	/// </summary>
	public class FlowNodeFactory<T> : IFlowNodeFactory where T : FlowNode
	{
		GCHandle _gcLock;

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void AddRef() {}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void Release() {}

		private void Free()
		{
			_gcLock.Free ();
		}

		/// <summary>
		/// Register this instance in CryEngine's FlowSystem. FlowNodes can be found by their name in Editor under category Mono.
		/// </summary>
		public static void Register()
		{
			var factory = new FlowNodeFactory<T> ();
			factory._gcLock = GCHandle.Alloc (factory);
			Env.FlowSystem.RegisterType("Mono:" + typeof(T).Name, new IFlowNodeFactoryPtr (factory));
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override IFlowNodePtr Create (IFlowNode.SActivationInfo info)
		{
			return new IFlowNodePtr(InternalFlowNode<T>.CreateInstance(info));
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void GetMemoryUsage (ICrySizer s) {}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void Reset () {}
	}

	/// <summary>
	/// Represents a specific FlowNide port type, which is dealt with separately from valuetypes. Signals are triggered rhather than holding a value.
	/// </summary>
	public class Signal
	{
		public bool Activate { get; set; } ///< Set to true of the parent FlowNode wants to fire the signal. 
	}

	/// <summary>
	/// Contains necessary data of a FlowNode port.
	/// </summary>
	public class PortConfig
	{
		public Type PortType { get; set; }
		public PropertyInfo Info { get; set; }
		public int Index { get; set; }
		public string Description { get; set; }

		public Type PropertyType { get { return Info.PropertyType; } }
		public string Name { get { return Info.Name; } }
	}

	/// <summary>
	/// Used by FlowNodes to define an input port.
	/// </summary>
	public class InputPortAttribute : Attribute
	{
		public string Description { get; private set; }

		public InputPortAttribute(string desc)
		{
			Description = desc;
		}
	}

	/// <summary>
	/// Used by FlowNodes to define an output port.
	/// </summary>
	public class OutputPortAttribute : Attribute
	{
		public string Description { get; private set; }

		public OutputPortAttribute(string desc)
		{
			Description = desc;
		}
	}

	/// <summary>
	/// Basic implementation of a FlowNode of Type T.
	/// </summary>
	public class InternalFlowNode<T> : IFlowNode where T : FlowNode
	{
		GCHandle _gcLock;
		int _refs;

		public T Node { get; private set; } ///< The public FlowNode instance for this CRYENGINE FlowNode.
		public IFlowNode.SActivationInfo Info { get; private set; } ///< Used by CryEngine internally.

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void AddRef()
		{
			_refs++;
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void Release()
		{
			if (0 >= --_refs) 
				_gcLock.Free ();
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public static IFlowNode CreateInstance(IFlowNode.SActivationInfo info)
		{
			var internalNode = new InternalFlowNode<T>();
			internalNode._gcLock = GCHandle.Alloc (internalNode);
			internalNode.Node = (T)Activator.CreateInstance (typeof(T), new object[]{});
			return internalNode;
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override IFlowNodePtr Clone(IFlowNode.SActivationInfo info)
		{
			return new IFlowNodePtr(CreateInstance(info));
		}

		/// <summary>
		/// Sorts all PortConfigAttributes into input and output values, for FlowNode configuration.
		/// </summary>
		public override void GetConfiguration(SFlowNodeConfig cfg)
		{
			SetupInputPorts(cfg, Node.Inputs);
			SetupOutputPorts(cfg, Node.Outputs);
			cfg.SetDescription (Node.Description);
			cfg.SetCategory((int)EFlowNodeFlags.EFLN_APPROVED);
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override bool SerializeXML(IFlowNode.SActivationInfo info, XmlNodeRef root, bool reading)
		{
			return true;
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void PostSerialize(IFlowNode.SActivationInfo info)
		{
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void ProcessEvent(IFlowNode.EFlowEvent evt, IFlowNode.SActivationInfo info)
		{
			Info = info;
			switch (evt)
			{
			case IFlowNode.EFlowEvent.eFE_Initialize:
				foreach (var pc in Node.Inputs) 
				{
					if (pc.PropertyType != typeof(Signal))
						pc.Info.SetValue (Node, GetPortValue (pc.PropertyType, pc.Index), null);
				}
				Info.pGraph.SetRegularlyUpdated (Info.myID, true);
				break;

			case IFlowNode.EFlowEvent.eFE_Activate:
				foreach (var pc in Node.Inputs) 
				{
					if (pc.PropertyType != typeof(Signal)) 
						pc.Info.SetValue (Node, GetPortValue (pc.PropertyType, pc.Index), null);
					else if (IsPortActive (pc.Index)) 
						Node.RaiseOnSignal (pc.Info);
				}
				break;

			case IFlowNode.EFlowEvent.eFE_Update:
				if (Node.OnQuit.Activate) 
				{
					Info.pGraph.SetRegularlyUpdated (Info.myID, false);
					Node.OnQuit.Activate = false;
					foreach (var pc in Node.Outputs) 
					{
						if (pc.PropertyType != typeof(Signal))
							ActivateOutput (pc.Index, pc.Info.GetValue (Node, null));
						else
							ActivateOutput (pc.Index);
					}
				}
				break;

			default:
				break;
			}
			Info = null;
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public override void GetMemoryUsage(ICrySizer s)
		{
		}

		/// <summary>
		/// Called by CryEngine. Do not call directly.
		/// </summary>
		public virtual void ProcessEvent(IFlowNode.EFlowEvent evt)
		{
		}

		/// <summary>
		/// Indicates whether a port has been activated.
		/// </summary>
		public bool IsPortActive(int portIndex)
		{
			return Info.GetInputPort(portIndex).IsUserFlagSet();
		}

		/// <summary>
		/// Passes information about input port configuration along to CryEngine.
		/// </summary>
		/// <param name="cfg">Configuration struct to be filled with information.</param>
		/// <param name="ports">Definition of input Ports.</param>
		protected void SetupInputPorts(SFlowNodeConfig cfg, List<PortConfig> ports)
		{
			cfg.SetInputSize (ports.Count);
			for (int i = 0; i < ports.Count; i++) 
			{
				var t = ports [i].PropertyType;
				if (t == typeof(string))
					cfg.AddStringInputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(Signal))
					cfg.AddVoidInputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(int))
					cfg.AddIntInputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(bool))
					cfg.AddBoolInputPort (i, ports[i].Name, ports[i].Description);
			}
		}

		/// <summary>
		/// Passes information about output port configuration along to CryEngine.
		/// </summary>
		/// <param name="cfg">Configuration struct to be filled with information.</param>
		/// <param name="ports">Definition of output ports.</param>
		protected void SetupOutputPorts(SFlowNodeConfig cfg, List<PortConfig> ports)
		{
			cfg.SetOutputSize (ports.Count);
			for (int i = 0; i < ports.Count; i++) 
			{
				var t = ports [i].PropertyType;
				if (t == typeof(string))
					cfg.AddStringOutputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(Signal))
					cfg.AddVoidOutputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(int))
					cfg.AddIntOutputPort (i, ports[i].Name, ports[i].Description);
				else if (t == typeof(bool))
					cfg.AddBoolOutputPort (i, ports[i].Name, ports[i].Description);
			}
		}

		/// <summary>
		/// Returns a typed value from a specific port, which was previously defined.
		/// </summary>
		/// <returns>The port value.</returns>
		/// <param name="portId">Port identifier.</param>
		/// <typeparam name="T">Type of the value.</typeparam>
		public object GetPortValue(Type t, int portIndex)
		{
			if (t == typeof(string)) 
				return ((object)Info.pGraph.GetPortString (Info, portIndex).c_str ());
			else if (t == typeof(int))
				return (object)Info.pGraph.GetPortInt(Info, portIndex);
			else if (t == typeof(bool))
					return (object)Info.pGraph.GetPortBool(Info, portIndex);
			return null;
		}

		/// <summary>
		/// Writes a specific value to one of the defines output ports.
		/// </summary>
		/// <param name="portId">Port identifier.</param>
		/// <param name="value">Value.</param>
		public void ActivateOutput(int portIndex, object value = null)
		{
			if (value == null)
				Info.pGraph.ActivateAnyOutput (Info.myID, portIndex);
			else if (value is string)
				Info.pGraph.ActivateStringOutput (Info.myID, portIndex, new CryString((string)value));
			else if (value is int)
				Info.pGraph.ActivateIntOutput (Info.myID, portIndex, (int)value);
			else if (value is bool)
				Info.pGraph.ActivateBoolOutput (Info.myID, portIndex, (bool)value);
		}
	}

	/// <summary>
	/// Base class for all Mono FlowNodes. Uses Reflection to find all input and output configs. Provides start and quit signals by default.
	/// </summary>
	public class FlowNode : MarshalByRefObject 
	{
		public static event EventHandler<FlowNode, PropertyInfo> OnSignal; ///< Raised whenever a signal port was activated.

		public void RaiseOnSignal (PropertyInfo signal) ///< Raises OnSignal from an external source.
		{
			if (OnSignal != null)
				OnSignal (this, signal);
		}

		public string Description { get; protected set; } ///< Shown as FlowNode description in Sandbox.
		public List<PortConfig> Inputs { get; private set; } ///< All InputPort attributed properties of this node.
		public List<PortConfig> Outputs { get; private set; } ///< All OutputPort attributed properties of this node.

		[InputPort("Trigger to start node")]
		public Signal OnStart { get; set; } ///< Is being activated if the node is called by a signal.
		[OutputPort("Trigger to quit node")]
		public Signal OnQuit { get; set; } ///< To be signaled if the FlowNode is supposed to quit.

		/// <summary>
		/// Initializes Input and Output ports using reflection.
		/// </summary>
		public FlowNode()
		{
			Inputs = new List<PortConfig> ();
			Outputs = new List<PortConfig> ();
			OnStart = new Signal ();
			OnQuit = new Signal ();

			var propertyInfos = GetType ().GetProperties ().ToList();
			foreach (var pi in propertyInfos) 
			{
				var ips = pi.GetCustomAttributes (typeof(InputPortAttribute), true).ToList();
				if (ips.Count > 0)
				{
					Inputs.Add (new PortConfig 
					{
						PortType = typeof(InputPortAttribute),
						Info = pi,
						Description = (ips [0] as InputPortAttribute).Description,
						Index = Inputs.Count
					});
				}
				var ops = pi.GetCustomAttributes (typeof(OutputPortAttribute), true).ToList();
				if (ops.Count > 0)
				{
					Outputs.Add (new PortConfig 
					{
						PortType = typeof(OutputPortAttribute),
						Info = pi,
						Description = (ops [0] as OutputPortAttribute).Description,
						Index = Outputs.Count
					});
				}
			}
			Description = "Default FlowNode";
		}

		/// <summary>
		/// Called internally. Do not call directly.
		/// </summary>
		public override object InitializeLifetimeService() 
		{
			return null;
		}

		/// <summary>
		/// Call to signal OnQuit. Will terminate the FlowNode execution.
		/// </summary>
		public void SignalQuit()
		{
			OnQuit.Activate = true;
		}
	}

	/// <summary>
	/// Scenes which are instanciated along the logical input of a RunScene FlowNode may use this base class for convenience.
	/// </summary>
	public class FlowScene<T> where T : FlowNode
	{
		public event EventHandler<T> OnSignalQuit; ///< Raised if the Scene signaled an end of its execution.*/

		/// <summary>
		/// To be called from deriving class.
		/// </summary>
		public T Node { get; private set; } ///< The FlowNode which has logical implications on this Scene.

		public FlowScene(T node)
		{
			Node = node;
		}

		/// <summary>
		/// Supposed to remove all instances that are not used outside the Scene.
		/// </summary>
		public virtual void Cleanup()
		{
		}

		/// <summary>
		/// Called by the Scene in order to make the RunScene FlowNode signal an end
		/// </summary>
		/// <param name="retVal">Optional Return value.</param>
		public void SignalQuit()
		{
			if (OnSignalQuit != null)
				OnSignalQuit (Node);
			Node.SignalQuit();
		}
	}
}
