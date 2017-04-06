
__kernel void vector_add_gpu (__global const float* src_a,  __global const float* src_b,  __global float* res,  const int num)  
{  
   /* get_global_id(0) ��������ִ�е�����̵߳�ID�� 
   ����̻߳���ͬһʱ�俪ʼִ��ͬһ��kernel�� 
   ÿ���̶߳����յ�һ����ͬ��ID�����Ա�Ȼ��ִ��һ����ͬ�ļ��㡣*/  
   const int idx = get_global_id(0);  
	//printf("%d\n",idx);
   /* ÿ��work-item�������Լ���id�Ƿ�����������������ڡ� 
   ����ڣ�work-item�ͻ�ִ����Ӧ�ļ��㡣*/  
   if (idx < num)  
      res[idx] = src_a[idx] + src_b[idx];  
}  