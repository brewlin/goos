# goos
<p>
<a href="https://github.com/brewlin/goos/issues"><img alt="GitHub issues" src="https://img.shields.io/github/issues/brewlin/goos"></a>
 <a href="https://github.com/brewlin/goos/network"><img alt="GitHub forks" src="https://img.shields.io/github/forks/brewlin/goos"></a>
 <a href="https://github.com/brewlin/goos/blob/master/LICENSE"><img alt="GitHub license" src="https://img.shields.io/github/license/brewlin/goos"></a>
 </p>
GPM 多线程协程调度器 for PHP Extension

## config & install
```asciidoc
# 编译php必须加上 ZTS支持
> wget https://www.php.net/distributions/php-7.3.5.tar.gz
> tar zxvf php-7.3.5.tar.gz & cd php-7.3.5
> ./configure --prefix=/path/to/  --enable-cli --with-config-file-path=/path/to/etc 
--sysconfdir=/path/to/etc --enable-maintainer-zts

# 编译扩展
> cd /path/to/goos
> php7.3.5-ize
> ./configure --with-php-config=php7.3.5-config
> make install
> echo 'extension=go.so' >> phpetcpath/php.ini 


```
## @G-M 多线程调度
- [x] 实现任意协程G创建后，自动绑定到线程M上去执行
- [x] 实现多线程协程G调度让出与恢复
- [ ] 优化php函数的拷贝与释放，更好的隔离内存申请与安全释放
- [ ] ... 
```php
<?php
Runtime::GOMAXPROCS(10);
for($i = 0;$i <100; $i++)
{
    go(function()use($i){
       go(function(){
            //echo "go yield  \n";
            //goyield();
            //echo "go end \n";
       });
    });
}
Runtime::wait();
```
## @G-P-M 任务窃取调度

## @G-P-M 信号抢占调度
- [x] 抢占调度，检测超过10ms持有G的线程，发起抢占调度
```php
<?php
//设置只有一个工作线程，在不抢占的情况下，永远无法触发 go 2
Runtime::GOMAXPROCS(1);
go(function(){
    for(;;) echo "go 1\n"; 
});
go(function(){
    for(;;) echo "go 2\n";
});
Runtime::wait();
```

