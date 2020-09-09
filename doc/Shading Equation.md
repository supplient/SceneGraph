# 参数转换
下文中会使用到$F_0,\rho_{ss}$，但我们在材质中不直接存储这两个参数，而是使用$c_{base},k_{metal},k_{IOR}$来转换得到。计算公式如下：

$$F_0=k_{metal}c_{base}+(1-k_{metal})\left(\frac{k_{IOR}-1}{k_{IOR}+1}\right)^2$$
$$\rho_{ss}=(1-k_{metal})c_{base}$$

$k_{metal}$表示的是材质的金属度，取值范围为[0,1]，为1时表示纯金属，为0时表示纯绝缘体。$k_{IOR}$为Index of Refraction，即折射率。$c_{base}$为表面基础颜色，其含义随$k_{metal}$的取值而变，当材质为金属时，它表示$F_0$，当材质为绝缘体时，它表示$\rho_{ss}$。

# Shading Equation
设最终从片元射向视点的光线的光照强度为$c_{out}$，则有

$$c_{out} = c_{indirect} + c_{light}$$ (1)

$c_{indirect}$模拟间接光源对反射到视点的光线的贡献，即从场景中其他物体表面反射过去的光线又经过当前物体表面反射回视点的光量。这里我们简单地使用下式计算：

$$c_{indirect}=L^{indirect}c_{base}$$ (1.5)

其中，$c_{base}$为表面的基础颜色，$L^{indirect}$为间接光源的光照强度。

$c_{light}$为场景中所有直接光源对反射到视点的光线的总贡献，其满足

$$c_{light}=\sum L_i^{out}(p,n,v)$$ (2)

其中，p为当前片元的坐标，n为法线向量，v为视线向量（即从当前片元射向视点的单位向量）。$L_i^{out}(p,n,v)$为第i个直接光源对以n为法线的表面在p点沿v点的出射光线的贡献。

按照反射公式计算$L_i^{out}(p,n,v)$：

$$L_i^{out}(p,n,v)=\int_{l\in\Omega}L_i^{in}(p,l)f(l,n,v)(n \cdot l)dl$$ (3)

其中，$L_i^{in}(p,l)$为第i个光源在p点以l为入射方向的入射光线的光照强度。$f(l,n,v)=f_{spec}(l,n,v)+f_{diff}(l,n,v)$为BRDF，表示出射光线和入射光线之间的比例，$f_{spec}$为高光部分的BRDF，即表面反射的BRDF。$f_{diff}$为漫反射部分的BRDF，即次表面散射的BRDF。$\Omega$为n方向上的单位半球面，$l$表示入射光线方向。

关于$L_i^{out}(p,n,v)$的计算，可以使用多种Shading Model:
1. [LTC方法](LTCModel.md)
2. [Phong Model](PhongModel.md)

