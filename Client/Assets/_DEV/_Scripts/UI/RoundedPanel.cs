using UnityEngine;
using UnityEngine.UI;
namespace Mimic.UI
{
    [AddComponentMenu("MIMIC/UI/Rounded Panel")]
    public sealed class RoundedPanel : MaskableGraphic
    {
        [SerializeField] private float radius = 20;
        protected override void OnPopulateMesh(VertexHelper vh)
        {
            vh.Clear(); var rect = rectTransform.rect;
            float round = Mathf.Min(radius, Mathf.Min(rect.width, rect.height) / 2);
            vh.AddVert(rect.center, color, Vector2.zero);
            int count = 0;
            for (int corner = 0; corner < 4; ++corner)
            {
                float cx = corner < 2 ? rect.xMax - round : rect.xMin + round;
                float cy = corner == 0 || corner == 3 ? rect.yMax - round : rect.yMin + round;
                for (int step = 0; step <= 8; ++step)
                {
                    float angle = (90 - corner * 90 - step * 90f / 8) * Mathf.Deg2Rad;
                    vh.AddVert(new Vector3(cx + Mathf.Cos(angle) * round, cy + Mathf.Sin(angle) * round), color, Vector2.zero);
                    ++count;
                }
            }
            for (int i = 1; i <= count; ++i) vh.AddTriangle(0, i, i == count ? 1 : i + 1);
        }
    }
}
