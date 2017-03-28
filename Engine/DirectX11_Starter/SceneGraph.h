#pragma once

#include <vector>

class Renderable;

class SceneGraph
{
public:
	Renderable*		CreateRenderable();

	void			ClearRenderables();

private:
	Renderable*		root;

	// TODO allocator for renderables
	std::vector<Renderable>		renderables;
};