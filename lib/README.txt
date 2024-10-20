This directory contains code from the following projects:

 * https://git.iem.at/zmoelnig/FST
 * https://github.com/steinbergmedia/vst3sdk
 * https://github.com/ODDSound/MTS-ESP

Note that this directory does not include the whole SDK packages, what is
included here is only what is required for compiling JS80P.

The _patches directory contains changes that have been applied to the original
code of these projects, usually to resolve compilation errors and to compile
them for platforms that they don't support officially. The name of the patch
files follows the "order-libname-subject.diff" format, where "order" is a
number which indicates the order in which the patches were applied, "libname"
refers to the name of the library, and "subject" is a short description of the
reason why the change was needed. The patches are in a Git compatible format.
The number of these patches should be kept to the absolute minimum: for
bugfixes, new features, and other improvements, a contribution (e.g. a merge
request) to the original upstream project is preferred.
