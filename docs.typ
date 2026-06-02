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

== Input/Output
The program must take in four arguments:

+ Filename of the input .PPM image.
+ Filename of the output .PPM image.
+ $K$ -- the minimum number of superpixels in the output.
+ $W$ -- maximum weight for merging superpixels.
