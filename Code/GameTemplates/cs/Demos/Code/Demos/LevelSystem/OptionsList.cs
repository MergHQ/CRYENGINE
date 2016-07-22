using System.Collections.Generic;
using System.Linq;

namespace CryEngine.LevelSuite
{
    /// <summary>
    /// Wraps a list of T and assigns a name to each value.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class OptionsList<T> where T : struct
    {
        public event EventHandler<OptionEvent> ValueChanged;

        public class OptionEvent
        {
            public T Value { get; set; }
            public string Name { get; set; }

            public OptionEvent(string name, T value)
            {
                Name = name;
                Value = value;
            }
        }

        private Dictionary<T, string> _options;
        private T? _currentOption;

        public OptionsList()
        {
            _options = new Dictionary<T, string>();
        }

        /// <summary>
        /// Adds the value and assigns the specified name.
        /// </summary>
        public void Add(T value, string name)
        {
            if (!_options.ContainsKey(value))
            {
                _options.Add(value, name);

                if (!_currentOption.HasValue)
                    _currentOption = value;
            }
        }

        /// <summary>
        /// Sets the value and fires the ValueChanged event.
        /// </summary>
        public void Set(T value)
        {
            if (_options.ContainsKey(value))
            {
                _currentOption = value;
                ValueChanged?.Invoke(new OptionEvent(_options[value], value));
            }
        }

        /// <summary>
        /// Moves to the next item in the list. If the current item is at the end, the next item will be the first item in the list.
        /// </summary>
        public void MoveNext()
        {
            CycleOptions(1);
        }

        /// <summary>
        /// Moves to the previous item in the list. If the current item is at the beginning, the next item will be the last item in the list.
        /// </summary>
        public void MovePrevious()
        {
            CycleOptions(-1);
        }

        private void CycleOptions(int direction)
        {
            if (!_currentOption.HasValue)
                return;

            var keys = _options.Keys.ToList();

            var indexOf = keys.IndexOf(_currentOption.Value);

            int i = 0;

            if (direction == 1)
            {
                // Wrap list when going forward. If we're at the end of the list, return to the start.
                if (indexOf == _options.Count - 1)
                {
                    i = 0;
                }
                else
                {
                    i = indexOf + 1;
                }
            }
            else
            {
                // Wrap list when going backwards. IF we're at the beginning of the list, return to the end.
                if (indexOf == 0)
                {
                    i = _options.Count - 1;
                }
                else
                {
                    i = indexOf - 1;
                }
            }

            Set(keys.ToList()[i]);
        }
    }
}
