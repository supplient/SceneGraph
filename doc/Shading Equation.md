设最终从片元射向视点的光线的光照强度为$c_{out}$，则有

$$c_{out} = c_{indirect} + c_{light}$$ (1)

其中，$c_{indirect}=(0.2, 0.2, 0.2)$为一定值，模拟间接光源，即从场景中其他物体表面反射过来的光线。

$c_{light}$为场景中所有直接光源对反射到视点的光线的总贡献，其满足

$$c_{light}=\sum L_i^{out}(p,n,v)$$ (2)

其中，p为当前片元的坐标，n为法线向量，v为视线向量（即从当前片元射向视点的单位向量）。$L_i^{out}(p,n,v)$为第i个直接光源对以n为法线的表面在p点沿v点的出射光线的贡献。而$L_i^{out}(p,n,v)$按照反射公式计算：

$$L_i^{out}(p,n,v)=\int_{l\in\Omega}L_i^{in}(p,l)f(l,n,v)(n \cdot l)dl$$ (3)

其中，$L_i^{in}(p,l)$为第i个光源在p点以l为入射方向的入射光线的光照强度。$f(l,n,v)=f_{spec}(l,n,v)+f_{diff}(l,n,v)$为BRDF，表示出射光线和入射光线之间的比例，$f_{spec}$为高光部分的BRDF，即表面反射的BRDF。$f_{diff}$为漫反射部分的BRDF，即次表面散射的BRDF。$\Omega$为n方向上的单位半球面，$l$表示入射光线方向。

# Punctual Light
Punctual Light，即为无限小的点的光源，其$L_i^{in}(p,l)$仅在一个$l$上有非零值，所以可得

$$L_i^{out}(p,n,v)=L_i^{in}(p,l)f(l,n,v)(n \cdot l)$$ (4)

其中，$L_i^{in}(p,l)$根据光源类型而变化，设$A_i$为第i个光源的光照强度的设定值，则

* 对于定向光，$l$固定不变，$L_i^{in}(p,l)=A_i$固定不变。
* 对于点光，设$p_i$为第i个光源的位置，$L_i^{in}(p,l)=k_{dist}(p)A_i$，其中$k_{dist}(p)=(\frac{r_0}{max(||p_l-p||,r_{min})})^2$。
* 对于聚光，设$w_i$为第i个光源的聚光方向，$\theta_{u},\theta_{penu}$分别为全影边界(umbra)关于聚光方向的角度和半影边界(penumbra)关于聚光方向的角度。$L_i^{in}(p,l)=k_{dir}(l)k_{dist}(p)A_i$，其中$l=p_i-p$，$k_{dir}=smoothstep(0,1,t)$, $t=\frac{w_i\cdot l-\cos\theta_{u}}{\cos\theta_{penu}-\cos\theta_{u}}$。

## Diffuse
漫反射部分假设表面为光滑表面（即表面的不规则处的大小要小于次表面散射距离），并假设使用Schlick approximation计算菲涅兹反射率$F(n,l)$（见后文），使用Shirley的方法：

$$f_{diff}(l,n,v)=\frac{21}{20\pi}(1-F_0)\rho_{ss}(1-(1-(n\cdot l)^+)^5) (1-(1-(n\cdot v)^+)^5)$$ (5)

## Specular
高光部分使用GGX分布，并使用LTC近似。

对于基于微面理论BRDF，设$h=\frac{l+v}{||l+v||}$表示微面法线，则有

$$f_{spec}(l,n,v)=\frac{F(h,l)G_2(l,h,v)D(h)}{4|n\cdot l||n\cdot v|}$$ (6)

其中，$F(h,l)$为菲涅兹反射率，使用Schlick approximation计算：

$$F(n,l)\approx F_0+(1-F_0)(1-(n\cdot l)^+)^5$$ (7)

$G_2(l,h,v)$为Smith height-correlated masking-shadowing function，其式为：

$$G_2(l,m,v)=\frac{\chi^+(m\cdot v)\chi^+(m\cdot l)}{1+\Lambda(v)+\Lambda(l)}$$ (8)

其中$\chi^+(x)=\begin{cases} 1,x>0 \\ 0,x\leq 0 \end{cases}$，而$\Lambda(s)$为随NDF而变的函数，GGX所用的NDF对应的$\Lambda(s)$为$\Lambda(s)=\frac{-1+\sqrt{1+\frac{1}{a^2}}}{2}$，其中$a=\frac{n\cdot s}{\alpha\sqrt{1-(n\cdot s)^2}}$，$\alpha$为粗糙度系数，取值为0时表示绝对光滑的表面，取值越大越粗糙。

$D(m)$为NDF，GGX所用的为：

$$
    D(m)=\frac{\chi^+(n\cdot m)\alpha^2}
    {\pi(1+(n\cdot m)^2(\alpha^2-1))^2}
$$(9)

### D(m)的计算
关于$D(m)$的计算，我们假设$n=(0,0,1)$，并假设m一定在z轴正半球，这可以通过变化到切空间以及裁剪来实现。

并设$m=(x,y,z), ||m||=1$，则$n\cdot m=z$。设$\theta=\arccos(n\cdot m)$，则$\tan^2\theta=\frac{x^2}{z^2}+\frac{y^2}{z^2}$，设$s_x=\frac{x}{z}, s_y=\frac{y}{z}$，则$\tan^2\theta=s_x^2+s_y^2$。

基于上述，有如下推导：

$$
    \begin{aligned}
    D(m)&=\frac{\alpha^2}{\pi (1+\cos^2\theta(\alpha^2-1))^2}\\
        &=\frac{\alpha^2}{\pi (1+2\cos^2\theta(\alpha^2-1)+\cos^4\theta(\alpha^2-1)^2)}\\
        &=\frac{\alpha^2}{\pi \cos^4\theta(\frac{1}{cos^2\theta}+\alpha^2-1)^2}\\
        &=\frac{\alpha^2}{\pi \cos^4\theta(\alpha^2+tan^2\theta)^2}     \left[\tan^2\theta=\frac{1}{\cos^2\theta}-1\right]\\
        &=\frac{1}{\pi\alpha^2\cos^4\theta} \left(\frac{1}{1+\frac{\tan^2\theta}{\alpha^2}}\right)^2\\
        &=\frac{1}{\pi\alpha^2z^4} \left(\frac{1}{1+\frac{s_x^2+s_y^2}{\alpha^2}} \right)^2\\
    \end{aligned}
$$

### LTC拟合BRDF
怎么拟合的我也不知道，不懂，直接拿LTC论文的demo的代码跑的。本节介绍在渲染等式中怎么使用近似后的LTC。

由式(6)，

$$
    \begin{aligned}
        f_{spec}(l,n,v)(n\cdot l)&=\frac{F(h,l)G_2(l,h,v)D(h)}{4|n\cdot v|} \\
        &\approx F(h,l)k^{amp}_{\alpha,v}T_{\alpha,v}(l)
    \end{aligned}
$$ (10)

其中，$T_{\alpha,v}(l)$为近似的LTC，其满足

$$
    T_{\alpha,v}(l)=\frac{(n\cdot M_{\alpha,v}^{-1}l)^+}{\pi}
$$ (11)

$M_{\alpha,v}$为拟合矩阵，存放在提前算好的纹理中。余弦瓣乘上$M_{\alpha,v}$后会近似于BRDF。设余弦瓣为

$$
    T_{\alpha,v}^0(l)=\frac{(n\cdot l)^+}{\pi}
$$ (12)

若设$l=M_{\alpha,v}l^0$，则有$T_{\alpha,v}(l)=T_{\alpha,v}^0(l^0)$。

另一方面，$k_{\alpha,v}^{amp}$为BRDF的模(norm)，即

$$
    k_{\alpha,v}^{amp}=\int_\Omega f_{sepc}(l,v)(n\cdot l)dl
$$ (13)

$k_{\alpha,v}^{amp}$与$M_{\alpha,v}$一样，也是存放在提前算好的纹理中。值得注意的是$\int_\Omega T^0(l)dl=1$，所以不用除以余弦瓣的模。


# Rect Light
还是按照式(3)反射公式进行计算，为了方便，在此复写一遍：

$$L_i^{out}(p,n,v)=\int_{l\in\Omega}L_i^{in}(p,l)f(l,n,v)(n \cdot l)dl$$

$L_i^{in}(p,l)$按照点光源的方式近似计算，将矩形的中心看作点光源的光源位置，使其与$l$无关，即

$$
    \begin{aligned}
        L_i^{in}(p)&=k_{dist}A_i\\
        k_{dist}&=\left(\frac{r_0}{max(||p_c-p||, 0)}\right)^2 \\
        p_c&=\frac{p_1+p_2+p_3+p_4}{4}
    \end{aligned}
$$

其中，$p_i$为矩形的四个顶点，关于光源朝向遵循右手法则。

代入反射公式得到，

$$
    L_i^{out}(p,n,v)=L_i^{in}(p) (\int_{l\in P_i}f_{spec}(l,n,v)(n \cdot l)dl + \int_{l\in P_i}f_{diff}(l,n,v)(n \cdot l)dl)
$$ (14)

其中，$P_i$为矩形围成的区域在n的单位半球面上的投影。

## Diffuse
近似为点光源的方式计算，将$p_c$看作光源位置，使用基本的Lambertian term，稍微考虑一下能量守恒：

$$
    f_{diff}(l,n,v)=(1-F(n,l))\frac{\rho_{ss}}{\pi}
$$

则

$$
    \int_{l\in P_i}f_{diff}(l,n,v)(n\cdot l)dl \approx f_{diff}(p_c-p,v)\int_{l\in P_i}(n\cdot l)dl
$$

而余弦的计算方法（BAUM, D. R., RUSHMEIER, H. E., AND WINGET, J. M. 1989. Improving radiosity solutions through the use of analytically de-termined form-factors. Computer Graphics (Proc. SIGGRAPH) 23, 3, 325–334）如下：

$$
    \int_{l\in P_i}=\frac{1}{2\pi} \sum_{k=1}^N\arccos(q_k^i\cdot q_t^i)\left( \left( \frac{q_k^i \times q_t^i}{||q_k^i \times q_t^i||} \right) \cdot \left( \begin{matrix}0\\0\\1 \end{matrix} \right) \right)
$$ (14)

其中$t=(k+1)\text{ mod }N$，$q_k^i$为构成$P_i$的顶点。

## Specular
菲涅尔反射率的计算近似为点光源进行计算，以$p_c$作为光源位置，使其与$l$无关：

$$
    \begin{aligned}
        \int_{l\in P_i}f_{spec}(l,v)(n\cdot l)dl &= \int_{l\in P_i}\frac{F(h,l)G_2(l,h,v)D(h)}{4|n\cdot v|} dl \\
        &\approx F(n, p_c-p) \int_{l\in P_i}\frac{G_2(l,h,v)D(h)}{4|n\cdot v|} dl
    \end{aligned}
$$

剩下的积分用LTC近似计算：

$$
    \begin{aligned}
        \int_{l\in P_i}\frac{G_2(l,h,v)D(h)}{4|n\cdot v|} dl &\approx k^{amp}_{\alpha, v} \int_{l\in P_i}T_{\alpha, v}(l)dl \\
        &= k^{amp}_{\alpha, v} \int_{l^0\in P^0_i}T^0_{\alpha, v}(l^0)dl^0 
    \end{aligned}
$$

其中，$P_i^0$为用$M_{\alpha, v}^{-1}$对$P_i$进行变换后的结果，即若设$q^{0, i}_k$为$P_i^0$的顶点，则$q_k^{0, i}=M_{\alpha,v}^{-1}q_k^i$。

代入得到：

$$
    \int_{l\in P_i}f_{spec}(l,v)(n\cdot l)dl 
    \approx \frac{F(n, p_c-p)k^{amp}_{\alpha, v}}{\pi} \int_{l^0\in P^0_i}n\cdot l^0 dl^0
$$

积分的计算方式同Diffuse部分式(14)。