using System;
using CryEngine.UI;

namespace CryEngine.LevelSuite.UI
{
    class FieldGroup : UIElement
    {
        private VerticalLayoutGroup _verticalLayout;
        

        public void OnAwake()
        {
            _verticalLayout = Instantiate<VerticalLayoutGroup>(this);
        }

        private void UpdateLayout()
        {
            _verticalLayout.RectTransform.Size = new Point(RectTransform.Width, RectTransform.Height);
        }

        
    }
}
