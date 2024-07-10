#include <stdio.h>

void printa_arr(int *, int);

int main()
{
    int arr[] = {5,1,4,2,3};
    int n = sizeof(arr)/sizeof(arr[0]);
    int aux;

    printa_arr(arr, n);
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n-1; ++j)
        {
            if (arr[j] > arr[j+1])
            {
                aux = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = aux;
            }
        }
    }
    printa_arr(arr, n);

    return 0;
}

void printa_arr(int *arr, int n)
{
    for (int i = 0; i < n; ++i)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}