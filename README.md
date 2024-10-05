# PaperDemos

## Reflective Shadow Map [Dachsbacher and Stamminger 2005]

[Dachsbacher, C. and Stamminger, M. 2005. Reflective shadow maps. Proceedings of the 2005 symposium on Interactive 3D graphics and games, Association for Computing Machinery, 203–231.
](https://dl.acm.org/doi/10.1145/1053427.1053460)

### Result

Using DX12 + Win32.

When taking samples on RSM, I use neighboring n x n texels for simplicity. Also screen space interpolation is not implemented.

Camera views:
![Result](Resources/ReflectiveShadowMap/img/result_combined.jpg)

RSM textures (left to right: depth, world position, normal, projected flux):
![RSM](Resources/ReflectiveShadowMap/img/rsm_combined.jpg)
