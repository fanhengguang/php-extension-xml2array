# php xml2array by php extension

php xml2array php 扩展实现， 即通过扩展的方式实现xml转换为array

## 版本
适用于php5

## 安装方式
```c
phpize 
export CFLAGS="-lxml2 -I/usr/include/libxml2"
./configure
make 
sudo make install
```

## 使用
```php
1 <?php
2 $file = 'xml.xml';
3 $content = file_get_contents($file);
4
5 $ret = xml2array($content);
6 var_dump($ret);
```



