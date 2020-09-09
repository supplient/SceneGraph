目前实现的Phong Model仅支持Punctual Light，所以可直接化为：
$$L_i^{out}(p,n,v)=L_i^{in}(p,l)f(l,n,v)(n \cdot l)$$

将其分成漫反射和高光两部分：

$$\begin{aligned}
L_i^{out}(p,n,v)
    &=L_i^{in}(p,l)(f_{diff}+f_{spec})(n \cdot l)
\end{aligned}$$

# 漫反射部分
我们直接使用次表面散射率作为漫反射部分的BRDF。

$$
f_{diff}=\rho_{ss}
$$

# 高光部分
高光部分使用菲涅兹反射率和针对粗糙率模拟微面法线分布的函数B来得到。

$F(h,l)$为菲涅兹反射率，使用Schlick approximation计算：

$$F(n,l)\approx F_0+(1-F_0)(1-(n\cdot l)^+)^5$$

微面法线分布函数B的计算如下：

$$B(m,n,h)=\frac{m+8}{8}((n\cdot h)^+)^m$$

其中$h=\frac{v+l}{||v+l||}$为半角向量，$m=1024(1-k_{rough})$为放大到[0,1024]范围内的光滑度系数。

由上述两个变量得到：
$$f_{spec}=F(n,l)B(m,n,h)$$