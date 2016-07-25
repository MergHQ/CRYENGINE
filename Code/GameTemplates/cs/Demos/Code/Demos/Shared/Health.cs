using CryEngine.EntitySystem;

namespace CryEngine.SampleApp
{
    public class Health : EntityComponent
    {
        public event EventHandler<float> ReceivedDamage;
        public event EventHandler NoHealth;

        public float CurrentHealth { get; private set; }
        public float MaxHealth { get; private set; }

        protected override void OnStart()
        {
            CurrentHealth = MaxHealth;
        }

        public void ApplyDamage(float damage)
        {
            if (CurrentHealth <= 0)
                return;

            CurrentHealth -= damage;

            ReceivedDamage?.Invoke(damage);

            if (CurrentHealth <= 0)
                NoHealth?.Invoke();

            CurrentHealth = MathExtensions.Clamp(CurrentHealth, 0, MaxHealth);
        }

        public void SetHealth(float health)
        {
            CurrentHealth = health;
            MaxHealth = health;
        }
    }
}
