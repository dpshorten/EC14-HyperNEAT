HyperNEAT EC class @VU 2014 
===========================
Documentation on HyperNEAT
---------------------------

Usage:
>1../Hypercube\_NEAT -I <experiment>.dat -R $RANDOM -O <outputname>  

>this will cause our version of the HyperNEAT tool to generate one individual with given output name. An individual consist of one .xml file (genotype) and .vxa file (phenotype)

>2../Hypercube\_NEAT -I <experiment>.dat -R $RANDOM -O <outputname> -ORG <individual01>  

>HyperNeat tool will mutate the given individual and output this in the output name  
 
>3../Hypercube\_NEAT -I <experiment>.dat -R $RANDOM -O <outputname> -ORG <individual01> <individual02>  
 
>HyperNeat tool will generate a new individual with given outputname, based on the genetic code in individual01 and individual02  

Explanation of the input arguments:

* -I Denotes the input file with experiment instructions
* -R The random seed
* -O The output file
* -ORG Existing additional input files (genotype only)


Additional info:

>This version of HyperNEAT will be used to research softrobot evolution without the use of an artificial fitness function. 
As such this version of HyperNEAT only supports reproduction of said softrobots.  

>$RANDOM is used so that it does not require to edit the parameter file each time and change the random seed value 

>[future link to paper?](www.vu.nl) 

![alt text](http://cdn9.staztic.com/app/i/1799/1799385/dna-wallpaper-407822-l-124x124.png)

