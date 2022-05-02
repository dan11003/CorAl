# CorAl
CorAl conference paper presented at  [ECMR](https://ieeexplore.ieee.org/document/9568846)
# Abstract
In robotics perception, numerous tasks rely on point cloud registration. However, currently there is no method that can automatically detect misaligned point clouds reliably and without environment-specific parameters. We propose "CorAl", an alignment quality measure and alignment classifier for point cloud pairs, which facilitates the ability to introspectively assess the performance of registration. CorAl compares the joint and the separate entropy of the two point clouds. The separate entropy provides a measure of the entropy that can be expected to be inherent to the environment. The joint entropy should therefore not be substantially higher if the point clouds are properly aligned. Computing the expected entropy makes the method sensitive also to small alignment errors, which are particularly hard to detect, and applicable in a range of different environments. We found that CorAl is able to detect small alignment errors in previously unseen environments with an accuracy of 95% and achieve a substantial improvement to previous methods.


# Relase

At the current time, the code is published as is without documentation


# Tested on
* Lidar
* Spinning radar

# Maintainers

Daniel Adolfsson (Daniel.adolfsson@oru.se)

shuo.sun (shuo.sun@oru.se)
