Trigger.prototype.checkGame = function(data)
{
    if(data.playerId == 1) // Player 1 won
	{
		Engine.WriteJSONFile("maps/scripts/fightData/fightData" + cmpTrigger.fightsCounter + ".json", 
		{
			"Winner" : "1",
		}
		);
		Engine.AppendToBuffer("Ready");
		Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	}
	else // Player 2 won
	{
		Engine.WriteJSONFile("maps/scripts/fightData/fightData" + cmpTrigger.fightsCounter + ".json", 
		{
			"Winner" : "2",
		}
		);
		Engine.AppendToBuffer("Ready");
		Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	}
};

{
    var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.fightsCounter = Engine.ReadFile("maps/scripts/fightsCounter.txt");
	Engine.AppendToBuffer("Not ready");
	Engine.WriteToFile("maps/scripts/fightStatus/gameState" + cmpTrigger.fightsCounter + ".txt");
	cmpTrigger.RegisterTrigger("OnPlayerWon", "checkGame", {"enabled" : true});
}