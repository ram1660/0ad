Trigger.prototype.checkTeams = function(data)
{	
	if(data.playerId == 1) // Player 1 won
	{
		Engine.WriteJSONFile("maps/scripts/fightData/fightData" + cmpTrigger.fightsCounter + ".json", 
		{
			"Player1_Units" : cmpTrigger.startingArmy1,
			"Player2_Units" : cmpTrigger.startingArmy2,
			"Winner" : "1",
			"UnitsLeft" : TriggerHelper.GetEntitiesByPlayer(1)
		}
		);
		Engine.AppendToBuffer("Ready");
		Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	}
	else // Player 2 won
	{
		Engine.WriteJSONFile("maps/scripts/fightData/fightData" + cmpTrigger.fightsCounter + ".json", 
		{
			"Player1_Units" : cmpTrigger.startingArmy1,
			"Player2_Units" : cmpTrigger.startingArmy2,
			"Winner" : "2",
			"UnitsLeft" : TriggerHelper.GetEntitiesByPlayer(2)
		}
		);
		Engine.AppendToBuffer("Ready");
		Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	}
};
{
	//Engine.SetSimRate(25.0);
	var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.fightsCounter = Engine.ReadFile("maps/scripts/fightsCounter.txt");
	Engine.AppendToBuffer("Not ready");
	Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	cmpTrigger.startingArmy1 = TriggerHelper.GetEntitiesByPlayer(1);
	cmpTrigger.startingArmy2 = TriggerHelper.GetEntitiesByPlayer(2);
	cmpTrigger.once = true;
	cmpTrigger.RegisterTrigger("OnPlayerWon", "checkTeams", {"enabled" : true});
}