/*
 * Axis.h
 *
 *  Created on: 27 нояб. 2015 г.
 *      Author: sadko
 */

#ifndef UI_AXIS_H_
#define UI_AXIS_H_

namespace lsp
{
    
    class Axis: public IGraphObject
    {
        private:
            enum flags_t
            {
                F_MIN           = 1 << 0,
                F_MAX           = 1 << 1,
                F_BASIS         = 1 << 2,
                F_LOGARITHMIC   = 1 << 3
            };

            size_t          nFlags;
            float           fDX;
            float           fDY;
            float           fMin;
            float           fMax;
            size_t          nWidth;
            size_t          nCenter;
            ColorHolder     sColor;
            IUIPort        *pPort;

        protected:
            void update();

        public:
            Axis(plugin_ui *ui);

            virtual ~Axis();

        public:
            virtual void draw(IGraphCanvas *cv);

            virtual void set(widget_attribute_t att, const char *value);

            virtual float actualMin();

            virtual float actualMax();

            virtual bool  isBasis();

            /** Apply axis transformation according to x and y
             *
             * @param cv canvas
             * @param x x coordinate (in pixels) of 2D-point to transform
             * @param y y coordinate (in pixels) of 2D-point to transform
             * @param dv delta-vector to apply for transform
             * @param count size of x, y and dv vector elements
             * @return true if values were applied
             */
            virtual bool apply(IGraphCanvas *cv, float *x, float *y, const float *dv, size_t count);

            /** Project the vector on the axis and determine it's value relative to the center
             *
             * @param cv canvas
             * @param x x coordinate (in pixels) of 2D-point on canvas
             * @param y y coordinate (in pixels) of 2D-point on canvas
             * @return the value after projectsion
             */
            virtual float project(IGraphCanvas *cv, float x, float y);

            /** Get parallel line equation
             *
             * @param x dot that belongs to parallel line
             * @param y dot that belongs to parallel line
             * @param a line equation
             * @param b line equation
             * @param c line equation
             */
            virtual bool parallel(float x, float y, float &a, float &b, float &c);

            /** Get rotated around the point angle
             *
             * @param x dot that belongs to line
             * @param y dot that belongs to line
             * @param angle rotation angle around dot
             * @param a line equation
             * @param b line equation
             * @param c line equation
             * @return
             */
            virtual bool angle(float x, float y, float angle, float &a, float &b, float &c);
    };

} /* namespace lsp */

#endif /* UI_AXIS_H_ */
