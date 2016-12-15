// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

using CryEngine.UI;
using CryEngine.UI.Components;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace CryEngine.Sydewinder
{
	/// <summary>
	/// Only allows data input via constructor to ensure one single place for all update operations
	/// </summary>
	public class TableData
	{
		/// <summary>
		/// The headers of the table, added in the defined order
		/// </summary>
		public IEnumerable<string> Headers { get; private set; }

		/// <summary>
		/// Contents of the Table, representation of rows
		/// </summary>
		public IEnumerable<string[]> Content { get; private set; }

		public int NumberOfTotalColumns { get; private set; }

		public TableData(IEnumerable<string> headers, IEnumerable<string[]> content)
		{
			if (headers == null)
				throw new ArgumentNullException("headers");
			
			if (content == null)
				throw new ArgumentNullException("content");
			
			Headers = headers;
			Content = content;

			NumberOfTotalColumns = headers.Count();
			int contentNo = (content.Count() == 0) ? 0 : content.Max(x => x.Count()); // Get longest content row.
			if (contentNo > NumberOfTotalColumns)
				NumberOfTotalColumns = contentNo;
		}
	}

	public class Table : Panel
	{
		private const float RowHeight = 66f;

		private TableData _dataSource = null;
		private bool _rightAlignLastColumn = false;
		private bool _requiresUpdate = true;
		private List<Text> _allTextElements = new List<Text>();

		public TableData DataSource
		{ 
			get { return _dataSource; }
			set
			{
				_dataSource = value;
				_requiresUpdate = true;
			}
		}

		public bool RightAlignLastColumn
		{ 
			get { return _rightAlignLastColumn; }
			set
			{
				_rightAlignLastColumn = value;
				_requiresUpdate = true;
			}
		}

		public float ContentHeight
		{ 
			get
			{
				if (_dataSource == null)
					return 0;

				return _dataSource.Content.Count() * RowHeight + RowHeight;
			}
		}

		public override void OnUpdate()
		{
			if (!_requiresUpdate || _dataSource == null)
				return;

			// Clear current contents.
			foreach(Text headerText in _allTextElements)
				this.RemoveComponent(headerText);
			
			_allTextElements.Clear();

			// Distribute text elements equally across the table.
			float widthPerText = this.RectTransform.Width / (float)_dataSource.NumberOfTotalColumns;

			string[] headers = _dataSource.Headers.ToArray();
			for (int i = 0; i < headers.Length; i++)
			{
				bool rightAligned = _rightAlignLastColumn && (i == headers.Length - 1);

				Text headerText = AddComponent<Text>();
				headerText.Content = headers[i];
				headerText.Color = Color.Gray;
				headerText.Alignment = rightAligned ? Alignment.TopRight : Alignment.TopLeft;
				headerText.Height = (byte)RowHeight;
				headerText.Offset = rightAligned ?  new Point(0, 0) : new Point(widthPerText * i, 0);

				_allTextElements.Add(headerText);
			}

			int rowNumber = 0;
			foreach(string[] contentRow in _dataSource.Content)
			{
				rowNumber++;

				for (int i = 0; i < contentRow.Length; i++)
				{
					bool rightAligned = _rightAlignLastColumn && (i == headers.Length - 1);

					Text contentText = AddComponent<Text>();
					contentText.Content = rightAligned ? (Convert.ToDouble(contentRow[i])).ToString ("N0", CultureInfo.GetCultureInfo ("de-DE")) : contentRow[i];
					contentText.Color = Color.White;
					contentText.Alignment = rightAligned ? Alignment.TopRight : Alignment.TopLeft;
					contentText.Height = 48;
					contentText.Offset = rightAligned ? new Point(0, rowNumber * RowHeight) : new Point(widthPerText * i, rowNumber * RowHeight);

					_allTextElements.Add(contentText);
				}
			}

			_requiresUpdate = false;
		}
	}
}
