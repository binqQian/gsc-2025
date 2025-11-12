import numpy as np

# 使用numpy生成随机比特矩阵（推荐 - 最快）
def generate_bit_matrix_numpy(rows=40000, cols=40000):
    """
    使用numpy生成随机比特矩阵
    内存占用约: 40000 * 40000 / 8 bytes = 200 MB (使用uint8存储每8个比特)
    """
    print(f"生成 {rows} x {cols} 随机比特矩阵...")
    
    # 生成0和1的随机矩阵
    bit_matrix = np.random.randint(0, 2, size=(rows, cols), dtype=np.uint8)
    
    print(f"矩阵形状: {bit_matrix.shape}")
    print(f"内存占用: {bit_matrix.nbytes / (1024**2):.2f} MB")
    
    return bit_matrix

if __name__ == "__main__":
    import time
    
    # 方法1: 最常用的方法
    start = time.time()
    matrix1 = generate_bit_matrix_numpy(40000, 40000)
    print(f"耗时: {time.time() - start:.2f} 秒")
    print(f"前5x5元素:\n{matrix1[:5, :5]}")
    print(f"矩阵中1的数量: {np.sum(matrix1)}")
    print()

    np.save('/data05/quanbin_data/bitMatrix/bit_matrix.npy', matrix1)
    print("矩阵已保存")