// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.Common;
using CryEngine.Components;
using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class SampleApp : Application
    {
        public void OnAwake()
        {
            // Load the example map.
            if (!Env.IsSandbox)
                Env.Console.ExecuteString("map Example");
        }
    }
}
