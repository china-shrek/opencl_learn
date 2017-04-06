
__kernel void vector_add_gpu (__global const float* src_a,  __global const float* src_b,  __global float* res,  const int num)  
{  
   /* get_global_id(0) 返回正在执行的这个线程的ID。 
   许多线程会在同一时间开始执行同一个kernel， 
   每个线程都会收到一个不同的ID，所以必然会执行一个不同的计算。*/  
   const int idx = get_global_id(0);  
	//printf("%d\n",idx);
   /* 每个work-item都会检查自己的id是否在向量数组的区间内。 
   如果在，work-item就会执行相应的计算。*/  
   if (idx < num)  
      res[idx] = src_a[idx] + src_b[idx];  
}  