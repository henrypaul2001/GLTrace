#pragma once
#include <glm/ext/vector_float3.hpp>
#include <algorithm>
#include "GLTMath.h"
#include <glm/geometric.hpp>
#include "AbstractShader.h"
class Camera {
public:
	int samples_per_pixel = 10;								// Number of random samples per pixel
	int max_bounces = 10;									// Maximum times a ray can bounce off of geometry
	glm::vec3 background_colour = glm::vec3(0.0);			// Scene background colour

	float vfov = 90.0f;										// Vertical field of view
	glm::vec3 lookfrom = glm::vec3(0.0f);					// Camera position
	glm::vec3 lookat = glm::vec3(0.0f, 0.0f, -1.0f);		// Point camera is looking at
	glm::vec3 vup = glm::vec3(0.0f, 1.0f, 0.0f);			// Camera relative up direction

	float defocus_angle = 0.0f;								// Variation angle of rays through each pixel
	float focus_dist = 10.0f;								// Distance from camera lookfrom point to plane of perfect focus

	void Initialise(const unsigned int viewWidth, const unsigned int viewHeight) {
		image_width = viewWidth;
		image_height = viewHeight;
		aspect_ratio = image_width / image_height;
		
		pixel_samples_scale = 1.0f / samples_per_pixel;
		
		sqrt_spp = int(std::sqrt(samples_per_pixel));
		pixel_samples_scale = 1.0f / (sqrt_spp * sqrt_spp);
		recip_sqrt_spp = 1.0f / sqrt_spp;

		// Determine viewport dimensions
		float theta = degrees_to_radians(vfov);
		float h = std::tan(theta / 2.0f);
		float viewport_height = 2.0 * h * focus_dist;
		float viewport_width = viewport_height * (double(image_width) / image_height);

		// Calculate u, v, w unit basis vectors for camera coordinate frame
		w = glm::normalize(lookfrom - lookat);
		u = glm::normalize(cross(vup, w));
		v = cross(w, u);

		// Calculate the vectors across viewport edges
		glm::vec3 viewport_u = viewport_width * u;
		glm::vec3 viewport_v = viewport_height * -v;

		// Pixel delta vectors
		pixel_delta_u = viewport_u / (float)image_width;
		pixel_delta_v = viewport_v / (float)image_height;

		// Upper left pixel location
		glm::vec3 viewport_upper_left = lookfrom - (focus_dist * w) - viewport_u / 2.0f - viewport_v / 2.0f;
		pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);

		// Calculate camera defocus disk basis vectors
		float defocus_radius = focus_dist * std::tan(degrees_to_radians(defocus_angle / 2.0f));
		defocus_disk_u = u * defocus_radius;
		defocus_disk_v = v * defocus_radius;
	}

	void SetUniforms(const AbstractShader& shader) const {
		shader.Use();
		shader.setInt("cam.image_width", image_width);
		shader.setInt("cam.image_height", image_height);
		shader.setFloat("cam.aspect_ratio", aspect_ratio);
		shader.setInt("cam.samples_per_pixel", samples_per_pixel);
		shader.setInt("cam.max_bounces", max_bounces);
		shader.setVec3("cam.background_colour", background_colour);

		shader.setFloat("cam.vfov", vfov);
		shader.setVec3("cam.lookfrom", lookfrom);
		shader.setVec3("cam.lookat", lookat);
		shader.setVec3("cam.vup", vup);

		shader.setFloat("cam.defocus_angle", defocus_angle);
		shader.setFloat("cam.focus_dist", focus_dist);
		
		shader.setFloat("cam.pixel_samples_scale", pixel_samples_scale);
		shader.setInt("cam.sqrt_spp", sqrt_spp);
		shader.setFloat("cam.recip_sqrt_spp", recip_sqrt_spp);
		shader.setVec3("cam.pixel00_loc", pixel00_loc);
		shader.setVec3("cam.pixel_delta_u", pixel_delta_u);
		shader.setVec3("cam.pixel_delta_v", pixel_delta_v);
		shader.setVec3("cam.u", u);
		shader.setVec3("cam.v", v);
		shader.setVec3("cam.w", w);
		shader.setVec3("cam.defocus_disk_u", defocus_disk_u);
		shader.setVec3("cam.defocus_disk_v", defocus_disk_v);
	}

private:
	unsigned int image_width, image_height;	// Rendered image width and height in pixel count
	float aspect_ratio;						// Ratio of image width over height
	float pixel_samples_scale;				// Colour scale factor for sum of pixel samples
	int sqrt_spp;							// Square root of number of samples per pixel
	float recip_sqrt_spp;					// 1 / sqrt_spp
	glm::vec3 pixel00_loc;					// Location of pixel 0, 0
	glm::vec3 pixel_delta_u;				// Offset to pixel to the right
	glm::vec3 pixel_delta_v;				// Offset to pixel below
	glm::vec3 u, v, w;						// Camera frame basis vectors
	glm::vec3 defocus_disk_u;				// Defocus disk horizontal radius
	glm::vec3 defocus_disk_v;				// Defocus disk vertical radius
};