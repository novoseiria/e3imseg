// SPDX-FileCopyrightText: Copyright (C) Nile Jocson <novoseiria@gmail.com>
// SPDX-License-Identifier: MPL-2.0

#import "@preview/charged-ieee:0.1.4": ieee

#show: ieee.with(
	title: "Image Segmentation using an MST",
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

+ Filename of the input .PPM image.
+ Filename of the output .PPM image.
+ $K$ -- the minimum number of superpixels in the output.
+ $W$ -- maximum weight for merging superpixels.



= Implementation
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
	n_"edges" = H(W - 1) + W(H - 1)
$

Where $W$ and $H$ are the width and height of the image, respectively. It then
iterates through all coordinates and calculates and pushes the `Edge` of the vertex
at the coordinates and the vertex to its right or below it. At the last column or the
last row, there is no vertex to the right or below, respectively, so no edge is
made to those endpoints. The fully constructed `PixelAdjGraph` is then returned.

== Subtask 2
Subtask 2 is centered around disjoint sets and MSTs. The disjoint sets are represented
by the `DisjointSet` class, where each superpixel is represented by a set, and
each set is represented by its root node. Disjoint sets were used since this gave
an efficient way to obtain the superpixel of a node in a graph, and an easy way
to merge superpixels.

The MST is represented simply using a `vector<Edge>`, which are all the edges in
the MST. To generate the MST, the algorithm in the project specifications were used,
which was identified as Borůvka's algorithm. I would imagine that this was selected
since it is easily parallelizable, an important characteristic for image processing.
This algorithm is implemented in the `graph::boruvka()` function.

== Subtask 3
Subtask 3 is similarly centered around disjoint sets and MSTs, however no MST is
actually generated since it isn't needed to create the output image.

The MST algorithm is Borůvka's algorithm but with three key modifications:

+ Two superpixels will only be merged if the minimum-weight edge between them is
	less than $W$.
+ Superpixel merging should be done in ascending order of edge weight.
+ The algorithm will terminate if there are $K$ superpixels left, or if the number
	of superpixels does not decrease in an iteration of the outer loop.

Modification 3 was made first, then modification 2, then modification 1. The number
of merges made is also counted in order to allow the termination if no merges were
done for modification 3. This modified algorithm is implemented in the `graph::segment()`
function. #footnote[I separated Mod. 4 from Subtask 3 as it was somewhat unrelated,
and I felt like it didn't make sense to include it.]
