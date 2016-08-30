using System;
using System.Linq;
using System.Collections.Generic;
using CryEngine.UI;

namespace CryEngine.Framework
{
    /// <summary>
    /// Lays out all active UIElements together as a group.
    /// </summary>
    public abstract class LayoutGroup : Panel
    {
        float spacing;
        List<UIElement> items;

        public float Spacing
        {
            get
            {
                return spacing;
            }
            set
            {
                spacing = value;
                UpdateLayout();
            }
        }

        public override void OnAwake()
        {
            base.OnAwake();
            items = new List<UIElement>();
        }

        public void Add(UIElement item)
        {
            if (!items.Contains(item))
            {
                items.Add(item);
                item.ActiveChanged += OnItemActiveChanged;
                UpdateLayout();
            }
        }

        public void Remove(UIElement item)
        {
            if (items.Contains(item))
            {
                items.Remove(item);
                item.ActiveChanged -= OnItemActiveChanged;
                UpdateLayout();
            }
        }

        public void Clear(bool updateLayout = true)
        {
            items.ForEach(x => x.ActiveChanged -= OnItemActiveChanged);
            items.Clear();

            if (updateLayout)
                UpdateLayout();
        }

        public void OnDestroy()
        {
            Clear(false);
        }

        void UpdateLayout()
        {
            OnUpdateLayout(items.Where(x => x.Active).ToList());
        }

        void OnItemActiveChanged(SceneObject arg)
        {
            UpdateLayout();
        }

        /// <summary>
        /// Called whenever an item is added or removed.
        /// </summary>
        /// <param name="items">All currently active UIElements.</param>
        protected abstract void OnUpdateLayout(List<UIElement> items);
    }
}

