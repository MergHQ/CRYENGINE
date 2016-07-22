using System;

namespace CryEngine.Towerdefense
{
    public class Resources
    {
        public event EventHandler<int> OnResourcesChanged;

        int val;

        public int Value
        {
            get
            {
                return val;
            }
            private set
            {
                val = value;
                if (val < 0)
                    val = 0;
                OnResourcesChanged?.Invoke(val);
            }
        }

        public Resources()
        {
            // Set initial resources
            Value = 1000;
        }

        public void Add(int amount)
        {
            Value += amount;
        }

        public void Set(int amount)
        {
            Value = amount;
        }

        public void Remove(int amount)
        {
            Value -= amount;
        }
    }
}

