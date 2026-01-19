
import base64
import sys
from cryptography.fernet import Fernet
from decrypted import config
class Decrypted:
  def __init__(self):
    self.cipher = Fernet(config.key.encode('utf-8'))
    self.namespace = {}

  def decrypt_and_execute(self, encrypted_data):
    """解密并执行代码，返回执行后的命名空间"""
    try:
      encrypted_bytes = base64.b64decode(encrypted_data.encode('utf-8'))
      decrypted_code = self.cipher.decrypt(encrypted_bytes)
      exec(decrypted_code.decode('utf-8'), self.namespace)
      return self.namespace
    except Exception as e:
      print(f"解密或执行失败: {str(e)}")
      sys.exit(1)

  def get_class_instance(self, class_name, *args, **kwargs):
    """获取解密代码中的类实例"""
    if class_name in self.namespace:
      cls = self.namespace[class_name]
      return cls(*args, **kwargs)
    else:
      raise AttributeError(f"类 '{class_name}' 未找到")

  def call_function(self, function_name, *args, **kwargs):
    """调用解密代码中的函数"""
    if function_name in self.namespace:
      func = self.namespace[function_name]
      return func(*args, **kwargs)
    else:
      raise AttributeError(f"函数 '{function_name}' 未找到")

  def decrypt_and_carStateHelp(self):
    self.decrypt_and_execute( config.carStateHelp)

  def decrypt_and_carcontrollerHelp(self):
    self.decrypt_and_execute( config.carcontrollerHelp)

