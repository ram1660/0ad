Engine.IncludeModule("common-api");
var badBot = (function() {
    var m = {};
    var dataFile;
    m.badBot = function badBot(settings) {
        API3.BaseAI.call(this, settings);
        this.testVar = 0;
        this.turn = 0;
        this.playedTurn = 0;
        //this.Config = new m.Config();
        //this.Config.updateDifficulty(settings.difficulty);
        //this.Config.personality = settings.personality;
        this.firstTime = true;
        this.savedEvents = {};
        this.defcon = 5;
        this.defconChangeTime = -10000000;
        this.fileIndexer = 0;
        this.fileAnswerIndexer = 0;
    };
    m.badBot.prototype = new API3.BaseAI();
    
    m.badBot.prototype.OnUpdate = function(sharedScript) {
        if (this.gameFinished)
        {
            return;
        }
		//API3.warn(this.testVar);
        if(this.testVar == 10)
        {
            Engine.AppendToBuffer(this.gameState.getEntities(1).toString());
            //Engine.WriteToFile("simulation/ai/badBot/asd.txt");
            //Engine.AppendToBuffer("I want to kill you");
            //Engine.WriteToFile("simulation/ai/badBot/ML/DataFiles/data" + this.fileIndexer.toString() + ".txt");
            //API3.warn("We wrote data to the fucking file!");
            this.testVar = 0;
            this.fileIndexer++;
            //warn(eval(this.gameState.getEntities(1)));
            //API3.warn(sharedScript.entities.toEntityArray());
        }
        if(Engine.FileExists("simulation/ai/badBot/ML/AnswerFiles/answer" + this.fileAnswerIndexer.toString() + ".txt") === true)
        {
            dataFile = Engine.ReadFile("simulation/ai/badBot/ML/AnswerFiles/answer" + this.fileIndexer.toString() + ".txt");
            //API3.warn(dataFile);
            this.fileAnswerIndexer++;
        }
        this.testVar++;
    };
	m.badBot.prototype.Deserialize = function(data, sharedScript)
	{
		this.isDeserialized = true;
		this.data = data;
		API3.warn(data);
    };
    return m;
}());