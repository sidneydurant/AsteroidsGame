Asteroids!
written by Sidney Durant

My game is an adaptation of Asteroids.

You play by dropping mines in front of asteroids to destroy them. When all the asteroids have been
destroyed, you win! You use the [w],[a],[s],[d] keys for movement, [p] to play or pause, and [space]
to drop mines during gameplay.

The two things I'm most proud of are the following:

My drawLine() function is an implementation of Bresenham's line algorithm. It can draw a line
between any two arbitrary points.

My pointInAsteroid() function solves the point in polgyon problem, returning true if the given point
is inside the polygon, or false if it is outside. It tests this by checking the number of collisions
between the ray shooting out of the given point and the polygon. The number of collisions determines
if the point is inside the polygon or outside. To optimize the code I first check for a bounding box
collision, if there is none, I use the more involved formula.

Thanks for reading!
Cheers,
-Sidney
