// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

using System;

namespace Statoscope
{
	interface ILogDataStream : IDisposable
	{
		int Peek();
		string ReadLine();
		bool IsEndOfStream { get; }
	}
}
