// SPDX-FileCopyrightText: Copyright (C) Nile Jocson <novoseiria@gmail.com>
// SPDX-License-Identifier: MPL-2.0

#import "@preview/charged-ieee:0.1.4": ieee

#show: ieee.with(
	title: "Image Segmentation using an MST Algorithm",
	authors: (
		(
			name: "Nile Jocson",
			department: [Electrical and Electronics Engineering Institute],
			organization: [University of the Philippines Diliman],
			location: [Quezon City, Philippines],
			email: "nile.xavier.jocson@eee.upd.edu.ph"
		),
	),
	figure-supplement: [Fig.]
)

#set figure(placement: top)

#set table(
	columns: (6em, auto),
	align: (left, right),
	inset: (x: 8pt, y: 4pt),
	stroke: (x, y) => if y <= 1 { (top: 0.5pt) },
	fill: (x, y) => if y > 0 and calc.rem(y, 2) == 0  { rgb("#efefef") },
)



= Source
The Git repository for this project is located at https://github.com/novoseiria/e3imseg.
Included are the license, source code, and documentation files.



= Specifications
== Subtasks
We are tasked with creating an image segmentation program working on PPM image
files. The program will be divided into subtasks in order to do this:

+ Generate a pixel adjacency graph from the image.
+ Run Borůvka's algorithm on the pixel adjacency graph.
+ Run the modified MST algorithm on the pixel adjacency graph, outputting a
	disjoint set to be used to create the output image.

== Input and Output
The program must take in four arguments:

+ Filename of the input .ppm image.
+ Filename of the output .ppm image.
+ $K$ -- the minimum number of superpixels in the output.
+ $W$ -- maximum weight for merging superpixels.



= Implementation
== Input
The arguments of the program are represented by the `Args` struct, which includes
a static factory function `from_os_args()` which will parse the `main` function
parameters `argc` and `argv` into an instance of itself, which is then returned.
Note that this function will throw if the number of arguments does not match what
is expected, or if any parsing error happened, e.g. from `stoi()` or `stod()`.

The `Args` struct contains the four arguments of the program:

- `input_filename: string` -- the filename of the input .ppm image.
- `output_filename: string` -- the filename of the output .ppm image.
- `k: int` -- corresponding to the $K$ variable.
- `w: double` -- corresponding to the $W$ variable.

== Subtask 1
Subtask 1 is centered around a pixel adjacency graph. This is implemented using
the `PixelAdjGraph` struct, which contains:

- `vertices: vector<Pixel>` -- the array of pixels, which are effectively
	the vertices of the graph.
- `edges: vector<Edge>` -- the array of the edges of the graph.

`Pixel` is a datatype given in the `ppm.h` header file, while `Edge` is a custom
struct representing the edges of a graph. This struct contains:

- `u: size_t` and `v: size_t` -- the endpoints of the edge. These correspond to
	the index of the endpoints of the edge in the `vertices` array.
- `weight: double` -- the weight of the edge.

Edge weight is calculated using the `pixel::weight()` function, which uses the
following formula:

$
	"weight"((v_i, v_j)) = (d_"rgb" (v_i, v_j))/255 ((d_"hue" (v_i, v_j))/360 + 0.5)
$

Where $d_"rgb" (u, v)$ is the average absolute difference of the RGB channels of the
two pixels, and $d_"hue" (u, v)$ is the circular difference between the hue values
of the two pixels.

An array was used to store the vertices since we need $O(1)$ access for calculating
the edge weights. An array was also used to store the edges because it is easily
sortable, which is needed for the modified MST algorithm in subtask 3. Both the
vertices and edges also have to be iterable.

`PixelAdjGraph` provides a static factory function `from_image()` which returns
an instance of the struct. The vertices are populated first by iterating through
the pixels of the image then repeatedly inserting into the `vertices` array. #footnote[
I only realized after submitting that I could've just used the vector constructor
from a pointer and a size in order to initialize the `vertices` array. Oh well.]
It then reserves the size of the `edges` array using the following equation:

$
	n_"edges" = h(w - 1) + w(h - 1)
$

Where $w$ and $h$ are the width and height of the image, respectively. It then
iterates through all coordinates and calculates and pushes the `Edge` of the vertex
at the coordinates and the vertex to its right or below it. At the last column or the
last row, there is no vertex to the right or below, respectively, so no edge is
made to those endpoints. The fully constructed `PixelAdjGraph` is then returned.
Note that this function will throw if no edges can be generated.

== Subtask 2
Subtask 2 is centered around disjoint sets and MSTs. The disjoint sets are represented
by the `DisjointSet` class, where each superpixel is represented by a set, and
each set is represented by its root node. Disjoint sets were used since this gave
an efficient way to obtain the superpixel of a node in a graph, and an easy way
to merge superpixels.

The `DisjointSet` class implements the two important operations of a disjoint set:
+ `root()` -- returns the root node index of a given node index. Does path compression.
+ `merge()` -- merges the two sets of the given node indices. Uses merge-by-size.

The MST is represented simply using a `vector<Edge>`, which contains all the edges selected
by the MST algorithm. To generate the MST, the algorithm in the project specifications was used,
which was identified as Borůvka's algorithm. I would imagine that this was selected
since it is easily parallelizable, an important characteristic for image processing.
This algorithm is implemented in the `graph::boruvka()` function.

== Subtask 3
Subtask 3 is similarly centered around disjoint sets and MSTs, however no MST is
actually generated since they aren't needed to create the output image. Instead, the
modified MST algorithm is simply used to mutate the disjoint set.

The MST algorithm is Borůvka's algorithm but with three key modifications:

+ Two superpixels will only be merged if the minimum-weight edge between them is
	less than $W$.
+ Superpixel merging should be done in ascending order of edge weight.
+ The algorithm will terminate if there are $K$ superpixels left, or if the number
	of superpixels does not decrease in an iteration of the outer loop.

Modification 3 was made first, then modification 2, then modification 1. The number
of merges made is also counted in order to allow the termination if no merges were
done for modification 3. This modified algorithm is implemented in the `graph::segment()`
function. #footnote[The project specifications also include a Mod. 4 for Subtask 3,
but since this was more related to output rather than the modified algorithm, it is
separated into a different section.]

== Output
In order to create the output image, three functions were made.

`graph::calculate_superpixel_info()`, the first function, returns a map containing
the index of the root of the superpixel as the key, and a `SuperpixelInfo` as the
value. Each `SuperpixelInfo` contains the number of pixels in the superpixel as well
as the total sum of the RGB channels of each pixel in the superpixel.

The second function, `pixel::average_pixel_map()` returns a map, similarly containing
the index of the root of the superpixel as the key, but this time with a `Pixel` as
the value. This function takes in the `SuperpixelInfo` map from the previous function,
and calculates the average `Pixel` value for each superpixel.

The third function, `ppm::write_segmented()` writes to the `PPMImage` the segmented
image data, which is comprised of the `DisjointSet` returned from subtask 3 and
the average pixel map from the previous function. The `PPMImage` has to be the same
width and height as the input image, so the loaded input image is actually mutated
in the program by this function.

The image is then saved using `ppm::save()` given the output filename. Note that
this function will throw on a failed save.

== Checking
In order to check the correctness of the code, I simply used the specified output
for each subtask to check against the given sample input and output. Additional
testing was done by verifying the output image given different values of $K$ and
$W$.
