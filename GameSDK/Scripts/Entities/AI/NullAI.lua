-- NullAI - Empty AI shell, very light-weight, used for standing in place of an empty AI entity container

Script.ReloadScript( "SCRIPTS/Entities/AI/NullAI_x.lua");
CreateActor(NullAI_x);
NullAI = CreateAI(NullAI_x);
NullAI:Expose();
NullAI:InitNullAI();
