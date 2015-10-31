# eds
Experimental data structures

eds::memmap
 - resembles std::vector, except
   for the case of resizing while containing a large amount of data it uses mremap calls instead of new-memcpy-delete sequence for resizing
   the methods push_front ; resize_front

