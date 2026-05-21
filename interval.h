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

        
};
inline const interval empty{+infinity, -infinity};
inline const interval universe{-infinity, +infinity};

#endif