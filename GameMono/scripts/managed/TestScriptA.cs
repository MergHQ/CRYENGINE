using CryEngine;

public class AnotherClass
{
}

public class TestScriptA : Component
{
	public int Number = 5;
	public int Number2 = 5;
	public AnotherClass AnotherClass;

	public void OnAwake()
	{
		Debug.Log ("TestScriptA:OnAwake");
	}
}
