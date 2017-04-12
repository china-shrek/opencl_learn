
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

void yuv420p_to_rgb24_pixel(const uchar y, const uchar u,const uchar v,__global uchar *rgb24)
{
	int R, G, B;
	R = y + 1.402*(v - 128);
	G = y - 0.34414*(u - 128) - 0.71414*(v - 128);
	B = y + 1.772*(u - 128);
	R = (R < 0 ? 0 : R > 255 ? 255 : R);
	G = (G < 0 ? 0 : G > 255 ? 255 : G);
	B = (B < 0 ? 0 : B > 255 ? 255 : B);
	rgb24[0] = R;
	rgb24[1] = G;
	rgb24[2] = B;
}

__kernel void Yuv420ToRGB24 (__global uchar *y,__global uchar *u,__global uchar *v,__global uchar *rgb24)  
{  
	int2 work_item_id		= (int2)(get_global_id(0),get_global_id(1));
	int2 work_item_size_Y	= (int2)(2,2);
	int yLinesize			= get_global_size(0) * work_item_size_Y.x;
	int uvLinesize			= get_global_size(0);
	int2 work_item_index_Y	= work_item_id * work_item_size_Y;

	uchar U = u[work_item_id.x + work_item_id.y * uvLinesize];
	uchar V = v[work_item_id.x + work_item_id.y * uvLinesize];

	int i,index,indexRGB;
	uchar Y;
	for(i=0;i<work_item_size_Y.x;i++)
	{
		index = work_item_index_Y.x + (work_item_index_Y.y + i) * yLinesize;
		Y = y[index];
		indexRGB = index * 3;
		yuv420p_to_rgb24_pixel(Y,U,V,&rgb24[indexRGB]);

		index += 1;
		Y = y[index];
		indexRGB = index * 3;
		yuv420p_to_rgb24_pixel(Y,U,V,&rgb24[indexRGB]);
	}
} 

__kernel void Yuv420ToRGB24_old (__global uchar *y,__global uchar *u,__global uchar *v,__global uchar *rgb24)  
{  
	int i		= get_global_id(0);
	int j		= get_global_id(1);  
	int width	= get_global_size(1);
	//int height	= get_global_size(0);
	
	int index = i * width + j;
	uchar Y = y[index];
	index = (i / 2) * (width / 2) + (j / 2);
	uchar U = u[index];
	uchar V = v[index];
	index = (i*width + j) * 3;
	//__global uchar *rgb = &rgb24[index];
	yuv420p_to_rgb24_pixel(Y,U,V,&rgb24[index]); 
} 