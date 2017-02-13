using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Common.BehaviorTree;

namespace CryEngine
{
	public class BehaviorTreeContext
	{
		#region Fields
		private uint _nodeId;
		private uint _entityId;
		private UpdateContext _updateContext;
		#endregion

		#region Properties
		public uint NodeId { get { return _nodeId; } }
		public uint EntityId { get { return _entityId; } }
		#endregion

		#region Constructors
		internal BehaviorTreeContext(uint nodeId, UpdateContext updateContext)
		{
			_nodeId = nodeId;
			_updateContext = updateContext;
			_entityId = updateContext.entityId;
		}

		internal BehaviorTreeContext(uint nodeId, EventContext eventContext)
		{
			_nodeId = nodeId;
			_entityId = eventContext.entityId;
		}
		#endregion

		#region Methods
		public void Log(string message)
		{
			#if (!RELEASE && (WIN32 || WIN64))
				if (_updateContext != null)
				{
					_updateContext.behaviorLog.AddMessage(message);
				}
			#endif
		}
		#endregion
	}

	public class BehaviorTreeContext<TRuntimeData> : BehaviorTreeContext
	{
		#region Fields
		private TRuntimeData _runtimeData;
		#endregion

		#region Properties
		public TRuntimeData RuntimeData { get { return _runtimeData; } }
		#endregion

		#region Constructors
		internal BehaviorTreeContext(uint nodeId, TRuntimeData runtimeData, UpdateContext updateContext) : base(nodeId, updateContext)
		{
			_runtimeData = runtimeData;
		}

		internal BehaviorTreeContext(uint nodeId, TRuntimeData runtimeData, EventContext eventContext) : base(nodeId, eventContext)
		{
			_runtimeData = runtimeData;
		}
		#endregion
	}

	public abstract class BehaviorTreeNodeBase : NodeProxy
	{
		internal BehaviorTreeNodeBase() { } // Make sure only types inside the Core assembly can inherit from it 
	}

	public abstract class BehaviorTreeNode : BehaviorTreeNodeBase
	{
		HashSet<UInt64> s_initialized = new HashSet<ulong>();

		#region Fields
		private static uint s_nextNodeId = 0;
		private uint _nodeId;
		private string _startLog;
		private string _successLog;
		private string _failureLog;
		private string _interruptLog;
		#endregion

		#region Properties
		public uint NodeId { get { return _nodeId; } }
		#endregion

		#region Constructors
		public BehaviorTreeNode() : base()
		{
			_nodeId = s_nextNodeId++;
		}
		#endregion
		
		#region Methods
		public sealed override Status Tick(UpdateContext context)
		{
			Status returnStatus = Status.Running;

			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			BehaviorTreeContext updateContext = new BehaviorTreeContext(_nodeId, context);
			if (!s_initialized.Contains(runtimeDataId))
			{
				if (!string.IsNullOrEmpty(_startLog))
				{
					updateContext.Log(_startLog);
				}
				returnStatus = OnInitialize(updateContext) ? Status.Running : Status.Failure;
			}

			if (returnStatus == Status.Running)
			{
				returnStatus = OnUpdate(updateContext);
			}

			if (returnStatus != Status.Running)
			{
				OnTerminate(updateContext);
				s_initialized.Remove(runtimeDataId);
			}

			if (returnStatus == Status.Success && !String.IsNullOrEmpty(_successLog))
			{
				updateContext.Log(_successLog);
			}
			else if (returnStatus == Status.Failure && !String.IsNullOrEmpty(_failureLog))
			{
				updateContext.Log(_failureLog);
			}

			return returnStatus;
		}

		public sealed override void Terminate(UpdateContext context)
		{
			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			if (s_initialized.Contains(runtimeDataId))
			{
				BehaviorTreeContext updateContext = new BehaviorTreeContext(_nodeId, context);
				OnTerminate(updateContext, true);
				s_initialized.Remove(runtimeDataId);

				if (!String.IsNullOrEmpty(_interruptLog))
				{
					updateContext.Log(_interruptLog);
				}
			}
		}

		public sealed override void HandleEvent(EventContext context, Event e)
		{
			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			if (s_initialized.Contains(runtimeDataId))
			{
				BehaviorTreeContext updateContext = new BehaviorTreeContext(_nodeId, context);
				HandleEvent(updateContext, e);
			}
		}

		public sealed override LoadResult LoadFromXml(XmlNodeRef xml, LoadContext context)
		{
			_startLog = xml.getAttr("_startLog");
			_successLog = xml.getAttr("_successLog");
			_failureLog = xml.getAttr("_failureLog");
			_interruptLog = xml.getAttr("_interruptLog");

			return OnLoadFromXml(xml, context);
		}

		private static UInt64 MakeRuntimeDataId(uint entityId, uint nodeId)
		{
			return (UInt64)entityId | (((UInt64)nodeId) << 32);
		}

		protected virtual bool OnInitialize(BehaviorTreeContext updateContext) { return true; }
		protected virtual Status OnUpdate(BehaviorTreeContext updateContext) { return Status.Success; }
		protected virtual void OnTerminate(BehaviorTreeContext updateContext, bool interrupted = false) { }
		protected virtual LoadResult OnLoadFromXml(XmlNodeRef xml, LoadContext context) { return LoadResult.LoadSuccess; }
		protected virtual void HandleEvent(BehaviorTreeContext updateContext, Event e) { }
		#endregion
	}

	public abstract class BehaviorTreeNode<TRuntimeData> : BehaviorTreeNodeBase where TRuntimeData : class, new()
	{
		#region Fields
		private static Dictionary<UInt64, TRuntimeData> s_runtimeData = new Dictionary<ulong, TRuntimeData>();
		private static uint s_nextNodeId = 0;
		private uint _nodeId;
		private string _startLog;
		private string _successLog;
		private string _failureLog;
		private string _interruptLog;
		#endregion

		#region Properties
		public uint NodeId { get { return _nodeId; } }
		#endregion

		#region Constructors
		public BehaviorTreeNode() : base()
		{
			_nodeId = s_nextNodeId++;
		}
		#endregion

		#region Methods
		public sealed override Status Tick(UpdateContext context)
		{
			Status returnStatus = Status.Running;

			#if (!RELEASE && (WIN32 || WIN64))
				context.debugTree.Push(this);
			#endif

			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			bool needsToBeInitialized = false;
			TRuntimeData runtimeData = GetRuntimeData(runtimeDataId, out needsToBeInitialized);
			BehaviorTreeContext<TRuntimeData> updateContext = new BehaviorTreeContext<TRuntimeData>(_nodeId, runtimeData, context);
			if (needsToBeInitialized)
			{
				if (!string.IsNullOrEmpty(_startLog))
				{
					updateContext.Log(_startLog);
				}
				returnStatus = OnInitialize(updateContext) ? Status.Running : Status.Failure;
			}

			if (returnStatus == Status.Running)
			{
				returnStatus = OnUpdate(updateContext);
			}

			if (returnStatus != Status.Running)
			{
				OnTerminate(updateContext);
				s_runtimeData.Remove(runtimeDataId);
			}

			if(returnStatus == Status.Success && !String.IsNullOrEmpty(_successLog))
			{
				updateContext.Log(_successLog);
			}
			else if(returnStatus == Status.Failure && !String.IsNullOrEmpty(_failureLog))
			{
				updateContext.Log(_failureLog);
			}

			#if (!RELEASE && (WIN32 || WIN64))
				context.debugTree.Pop(returnStatus);
			#endif


			return returnStatus;
		}

		public sealed override void Terminate(UpdateContext context)
		{
			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			if (s_runtimeData.ContainsKey(runtimeDataId))
			{
				TRuntimeData runtimeData = s_runtimeData[runtimeDataId];
				BehaviorTreeContext<TRuntimeData> updateContext = new BehaviorTreeContext<TRuntimeData>(_nodeId, runtimeData, context);
				OnTerminate(updateContext, true);
				s_runtimeData.Remove(runtimeDataId);

				if(!String.IsNullOrEmpty(_interruptLog))
				{
					updateContext.Log(_interruptLog);
				}
			}
		}

		public sealed override void HandleEvent(EventContext context, Event e)
		{
			UInt64 runtimeDataId = MakeRuntimeDataId(context.entityId, _nodeId);
			if (s_runtimeData.ContainsKey(runtimeDataId))
			{
				TRuntimeData runtimeData = s_runtimeData[runtimeDataId];
				BehaviorTreeContext<TRuntimeData> updateContext = new BehaviorTreeContext<TRuntimeData>(_nodeId, runtimeData, context);
				HandleEvent(updateContext, e);
			}
		}

		public sealed override LoadResult LoadFromXml(XmlNodeRef xml, LoadContext context)
		{
			_startLog = xml.getAttr("_startLog");
			_successLog = xml.getAttr("_successLog");
			_failureLog = xml.getAttr("_failureLog");
			_interruptLog = xml.getAttr("_interruptLog");

			return OnLoadFromXml(xml, context);
		}

		private static UInt64 MakeRuntimeDataId(uint entityId, uint nodeId)
		{
			return (UInt64)entityId | (((UInt64)nodeId) << 32);
		}

		private static TRuntimeData GetRuntimeData(UInt64 runtimeDataId, out bool justCreated)
		{
			justCreated = false;
			if (!s_runtimeData.ContainsKey(runtimeDataId))
			{
				s_runtimeData.Add(runtimeDataId, new TRuntimeData());
				justCreated = true;
			}
			return s_runtimeData[runtimeDataId];
		}

		protected virtual bool OnInitialize(BehaviorTreeContext<TRuntimeData> updateContext) { return true; }
		protected virtual Status OnUpdate(BehaviorTreeContext<TRuntimeData> updateContext) { return Status.Success; }
		protected virtual void OnTerminate(BehaviorTreeContext<TRuntimeData> updateContext, bool interrupted = false) { }
		protected virtual LoadResult OnLoadFromXml(XmlNodeRef xml, LoadContext context) { return LoadResult.LoadSuccess; }
		protected virtual void HandleEvent(BehaviorTreeContext<TRuntimeData> updateContext, Event e) {  } 
		#endregion
	}
}