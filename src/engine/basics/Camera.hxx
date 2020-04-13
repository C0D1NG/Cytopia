#ifndef CAMERA_HXX_
#define CAMERA_HXX_

#include "SDL.h"
#include "point.hxx"

class Camera
{
public:
  /// the size of the default floor tiles.
  static SDL_Point tileSize;
  /// the offset of the camera.
  static SDL_Point cameraOffset;
  /// the zoom level of the camera.
  static double zoomLevel;

  /** \brief Centers camera on given isometric coordinates
  * Centers the camera on the given isometric coordinates.
  * @param Point object - coordinates to center the camera on
  */
  static void centerScreenOnPoint(const Point &isoCoordinates);

  /** \brief Centers camera on the middle of the map
  * Centers the camera on the middle of the map.
  */
  static void centerScreenOnMapCenter();

  /** \brief Increases the zoom level of the camera
  * Increases the zoom level of the camera.
  */
  static void increaseZoomLevel();

  /** \brief Decreases the zoom level of the camera
  * Decreases the zoom level of the camera.
  */
  static void decreaseZoomLevel();

  static void setPinchDistance(float pinchDistance, int isoX, int isoY);

  static Point centerIsoCoordinates;

private:
  Camera() = delete;
  ~Camera() = delete;

  static float m_pinchDistance;
};

#endif
