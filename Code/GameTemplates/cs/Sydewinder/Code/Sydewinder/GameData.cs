// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using System;
using System.IO;
using System.Runtime.Serialization;
using System.Linq;
using System.Collections.Generic;

namespace CryEngine.Sydewinder
{
	[DataContract]
	public class GameData
	{
		[DataMember]
		public string Name { get; set;}
		[DataMember]
		public double Score { get; set;}
	}

	[DataContract]
	public class Highscore
	{
		private const int MAX_COUNT = 7;
		private string _url = null;

		/// <summary>
		/// The highscore of the current game
		/// </summary>
		public static Highscore CurrentScore = null;

		[DataMember]
		public List<GameData> ScoreList = new List<GameData>();

		/// <summary>
		/// Initializes a new instance of the <see cref="CryEngine.Sydewinder.Highscore"/> class
		/// </summary>
		/// <param name="url">URL</param>
		public Highscore(string url)
		{
			_url = url; 
		}

		/// <summary>
		/// Loads the highscore from the provided file or creates an empty highscore if the file doesn't exist. The highscore is afterwards accessible via 'CurrentScore'
		/// </summary>
		/// <param name="url">URL to the file</param>
		public static void InitializeFromFile(string url)
		{
			if (File.Exists(url))
			{
				try
				{
					CurrentScore = Tools.FromJSON<Highscore>(File.ReadAllText(url));
					CurrentScore._url = url;
				}
				catch
				{
					CurrentScore = new Highscore (url);
				}
			}				
			else
				CurrentScore = new Highscore(url);

			// Automatically order descending by score
			CurrentScore.ScoreList = CurrentScore.ScoreList.OrderByDescending(x => x.Score).ToList();
		}

		public bool TryAddScore(GameData data)
		{
			double minValue = (ScoreList.Count == 0) ? 0d : ScoreList.Min(x => x.Score);

			// Insert score if it is high enough compared to other scores.
			if (ScoreList.Count < MAX_COUNT || minValue < data.Score) 
			{
				ScoreList.Add(data);

				// Re-Order Score list
				ScoreList = ScoreList.OrderByDescending(x => x.Score).ToList();

				if (ScoreList.Count > MAX_COUNT)
					ScoreList.RemoveAt (MAX_COUNT);
				
				return true;
			}
			return false;
		}

		public void StoreToFile()
		{
			var data = Tools.ToJSON(this);
			if (File.Exists(_url))
				File.SetAttributes(_url, FileAttributes.Normal);
			File.WriteAllText(_url, data);
		}
	}
}
