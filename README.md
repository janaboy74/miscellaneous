# My miscellaneous projects

## XSpeed
![xspeed-screenshot](https://github.com/janaboy74/miscellaneous/assets/54952408/1b8b95cc-5ede-40d1-ae99-8cb648a3c86f)
- Windows demo application with GDIPlus and audio handling functions for stationary bicycle.
## binaryinterleave
- Sorting algorithm with threads.
## checkspd
- My old speed checker app.
## endian
- Easy check the endianness of the cpu / os.
## general
### splinetest
- Little test for spline algorithms.
## pathfind2
- Simple path finder
## peid
- Check weither the windows exe is 32 or 64 bit.
## volfix
- Simple mp3 volume normalizer for windows with gui.
## SVGGear
- Create real gears for 3D graphics / printing ( for e.g. in Blender ) and also for 2D graphics<br/>
Some examples:<br/>
./svggear -o "gear.svg" -r 47.8 -d 11 -c 20 -z 370 -st 4 -sb 4 -sc 7 -b 0.4 -gt 2 -gb 2 -gc 3<br/>
./svggear -o "gear-fill.svg" -r 47.8 -d 11 -c 20 -f -v -z 370 -st 4 -sb 4 -sc 7 -b 0.4 -gt 2 -gb 2 -gc 3<br/>
./svggear -o "gear-inv.svg" -i -r 47.8 -d 11 -c 20 -z 370<br/>
./svggear -o "gear-rack.svg" -r 47.8 -l 3 -d 11 -c 20 -z 370 -st 2 -sb 2 -sc 7 -b 0.6 -gt 2 -gb 3 -gc 5<br/>
use -h option for help / description.
### Recommended steps in blender:<br/>
- import -> s.v.g.
- click / select object
- right click -> Convert to -> Mesh
- [tab] (edit mode)
- face select ( somewhere in the top-left corner of toolbox )
- [a] - select all
- [ctrl] + [x] - dissolve faces<br/>
#### optional:
- [e] - extrude face - to add volume for the object( [ctrl] for snapping to round values )<br/>
#### Then you can export into stl and print with a 3D Printer.<br/>
## weather-calendar
![weather-screenshot](https://github.com/janaboy74/miscellaneous/assets/54952408/c1374512-609f-4ac3-b8bf-9c76b2acf31d)
- Weather and Calendar app using gtkmm.
