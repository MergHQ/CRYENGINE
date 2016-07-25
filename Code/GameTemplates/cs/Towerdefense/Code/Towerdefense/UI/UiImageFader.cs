using System;
using System.Collections.Generic;
using CryEngine.Common;
using CryEngine.Components;
using CryEngine.UI;
using CryEngine.UI.Components;

namespace CryEngine.Towerdefense
{
    public class UiImageFader : Component
    {
        bool doFade;
        float duration = 2f;
        float timeStamp;
        List<Image> images;

        public void OnAwake()
        {
            images = new List<Image>();
        }

        public void Add(Image image)
        {
            images.Add(image);
        }

        public void Remove(Image image)
        {
            images.Remove(image);
        }

        /// <summary>
        /// Fades the images in and out over the specified duration of time.
        /// </summary>
        /// <param name="duration">Duration.</param>
        public void FadeInOut(float duration)
        {
            doFade = true;
            timeStamp = Global.gEnv.pTimer.GetCurrTime();
            this.duration = duration;
        }

        public void OnUpdate()
        {
            if (doFade)
            {
                if (Global.gEnv.pTimer.GetCurrTime() < timeStamp + duration)
                {
                    var t = Global.gEnv.pTimer.GetCurrTime() / 0.5f;
                    images.ForEach(x => x.Color.A = Math.Abs(1f * (float)Math.Sin(t)));
                }
                else
                {
                    images.ForEach(x => x.Color.A = 1f);
                    doFade = false;
                }
            }
        }

        public void OnDestroy()
        {
            images.Clear();    
        }
    }
}

