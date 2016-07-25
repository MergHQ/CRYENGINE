using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using CryEngine.Components;
using CryEngine.Framework;


namespace CryEngine.Towerdefense.Components
{
    public class Health : GameComponent
    {
        /// <summary>
        /// Called when the Health or MaxHealth is changed.
        /// </summary>
        public event EventHandler OnHealthChanged;

        /// <summary>
        /// Called when damage is applied, passing in the amount of damage recieved.
        /// </summary>
        public event EventHandler<int> OnDamageReceived;

        /// <summary>
        /// Called when health reaches 0.
        /// </summary>
        public event EventHandler OnDeath;

        public int CurrentHealth { get; private set; }
        public int MaxHealth { get; private set; }

        public void ApplyDamage(int damage)
        {
            // Don't allow damage to be applied when health is already 0.
            if (CurrentHealth == 0)
                return;

            SetHealth(CurrentHealth - damage);

            OnDamageReceived?.Invoke(damage);
            OnHealthChanged?.Invoke();

            if (CurrentHealth <= 0)
            {
                CurrentHealth = 0;
                OnDeath?.Invoke();
            }
        }

        public void SetHealth(int health)
        {
            CurrentHealth = MathEx.Clamp(health, 0, MaxHealth);
            OnHealthChanged?.Invoke();
        }

        public void SetMaxHealth(int maxHealth)
        {
            MaxHealth = MathEx.Clamp(maxHealth, CurrentHealth, 9999);
            OnHealthChanged?.Invoke();
        }
    }
}
