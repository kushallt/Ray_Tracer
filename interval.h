#ifndef INTERVAL_H
#define INTERVAL_H

class interval{
    public:
        double mint, maxt;
        interval(double mint, double maxt) : mint(mint), maxt(maxt) {}
        bool contains(double t) const{
            return t>=mint && t<=maxt;
        }
        double surrounds(double t) const{
            return t > mint && t < maxt;
        }
        double clamp(double x) const {
        if (x < mint) return mint;
        if (x > maxt) return maxt;
        return x;
    }

        static const interval empty, universe;
};
const interval interval::empty = interval(+infinity, -infinity);
const interval interval::universe = interval(-infinity, +infinity);

#endif