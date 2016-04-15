Script.ReloadScript( "Scripts/Entities/actor/player.lua");
-----------------------------------------------------------------------------------------------------

DummyPlayer = new(Player);

CreateActor(DummyPlayer);
DummyPlayer:Expose();

