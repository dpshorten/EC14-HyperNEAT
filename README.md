HyperNEAT
=========

This is a first draft of a README - don't take it too seriously!

This repo is a fork of the HyperNEAT distribution that was used by the Creative Machines Lab (TODO: insert link here). They in turn got it from MisterTea (TODO: also insert link, he is also on Github).
We will modify this version of the Hyperneat binary to work with VoxCAD/voxelyze and to have 3 features:

- run hyperneat binary without any parameters: creates new random individual (Voxel-based organism, like in the paper "Unshackling Evolution:..." by Cheney & Co.)
- run hn binary with one parameter (filename of a .vxa Voxel-square): mutates that individual and writes new individual
- run hn binary with two parameters (both .vxa filenames): create a new .vxa organism as offspring from the two input parents

