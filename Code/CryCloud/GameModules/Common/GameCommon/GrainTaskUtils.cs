using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;
using System.Threading;
using System.Xml.Linq;
using System.Net;
using Orleans.Runtime;

namespace CryBackend
{
	public static class GrainTaskUtils
	{
		public delegate Task ForkedInGrainTaskDeleg(CancellationToken ctoken);

		private struct InGrainTaskState
		{
			public Dictionary<string, object> Context { get; set; }
			public CancellationToken CancelToken { get; set; }
		}


		public static void Fork(ForkedInGrainTaskDeleg body, Dictionary<string, object> ctx, CancellationToken ctoken,
			Action<int, string, Exception> logger)
		{
			var state = new InGrainTaskState() {Context = ctx, CancelToken = ctoken};
			Task.Factory.StartNew(
					async _state =>
					{
						RequestContext.Import(((InGrainTaskState) _state).Context);
						await body(((InGrainTaskState) _state).CancelToken);
					},
					state,
					ctoken)
				.Unwrap()
				.ForgetAndLog(logger);
		}

	}
}