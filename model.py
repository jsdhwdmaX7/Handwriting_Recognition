import tensorflow as tf
from tensorflow.keras import layers, models


def create_model():
    inputs = layers.Input(shape=(28, 28), name='input')

    # LSTM层: 28个时间步，20个单元，返回完整序列
    # 输入 (28,28) 自动解释为 28个时间步 × 28个特征
    x = layers.LSTM(
        units=20,
        return_sequences=True,  # 返回 (28, 20)
        activation='tanh',
        recurrent_activation='sigmoid',
        use_bias=True,
        name='lstm_0'
    )(inputs)

    # 展平: (28, 20) -> 560
    x = layers.Reshape((560,), name='reshape')(x)

    # 全连接层: 560 -> 10
    x = layers.Dense(10, use_bias=True, name='dense')(x)

    # Softmax输出
    outputs = layers.Activation('softmax', name='output')(x)

    model = models.Model(inputs=inputs, outputs=outputs, name='trained_lstm')

    return model


# ============================================================================
# 数据预处理
# ============================================================================

def preprocess_data(x, y):
    x = x.astype('float32')
    return x, y


# ============================================================================
# 训练模型
# ============================================================================

def train_model(epochs=10, batch_size=32):
    """
    训练模型

    参数:
        epochs: 训练轮数
        batch_size: 批次大小
    """

    # 加载MNIST数据集
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

    # 预处理（归一化到0-1范围）
    x_train, y_train = preprocess_data(x_train, y_train)
    x_test, y_test = preprocess_data(x_test, y_test)

    # 创建模型
    model = create_model()

    # 打印模型结构
    print("模型结构:")
    model.summary()

    # 编译模型
    model.compile(
        optimizer='adam',
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )

    # 训练
    print(f"\n开始训练 (epochs={epochs}, batch_size={batch_size})...")
    history = model.fit(
        x_train, y_train,
        batch_size=batch_size,
        epochs=epochs,
        validation_data=(x_test, y_test),
        verbose=1
    )

    # 评估
    test_loss, test_acc = model.evaluate(x_test, y_test, verbose=0)
    print(f"\n测试准确率: {test_acc:.4f}")

    return model, history


# ============================================================================
# 保存模型
# ============================================================================

def save_model(model, filepath='model.h5'):
    """保存模型"""
    model.save(filepath)
    print(f"模型已保存: {filepath}")


# ============================================================================
# 使用示例
# ============================================================================

if __name__ == "__main__":
    # 训练模型
    model, history = train_model(epochs=10, batch_size=32)

    # 保存模型
    save_model(model, 'model.h5')

    # 可选：转换为TFLite
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    with open('model.tflite', 'wb') as f:
        f.write(tflite_model)
    print("已保存: model.tflite")