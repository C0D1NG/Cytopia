#include "EventManager.hxx"

#include "basics/Camera.hxx"
#include "basics/isoMath.hxx"
#include "basics/mapEdit.hxx"
#include "basics/Settings.hxx"
#include "basics/GameStates.hxx"
#include "common/enums.hxx"
#include "map/MapLayers.hxx"
#include "Map.hxx"
#include "Sprite.hxx"

#include "LOG.hxx"

#ifdef MICROPROFILE_ENABLED
#include "microprofile.h"
#endif

void EventManager::unHighlightNodes()
{
  for (auto node : m_nodesToPlace)
  {
    Engine::instance().map->unHighlightNode(node);
  }
  for (auto node : m_nodesToHighlight)
  {
    Engine::instance().map->unHighlightNode(node);
  }
  m_nodesToHighlight.clear();
  m_nodesToPlace.clear();
}

void EventManager::checkEvents(SDL_Event &event, Engine &engine)
{
#ifdef MICROPROFILE_ENABLED
  MICROPROFILE_SCOPEI("EventManager", "checkEvents", MP_BEIGE);
#endif
  // check for UI events first
  SDL_Point mouseScreenCoords;
  Point mouseIsoCoords{};

  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
    case SDL_QUIT:
      engine.quitGame();
      break;

    case SDL_KEYDOWN:
      switch (event.key.keysym.sym)
      {
      case SDLK_ESCAPE:
        // TODO: Toggle last opened menu or settings menu if nothing is open
        m_uiManager.toggleGroupVisibility("PauseMenu");
        break;

      case SDLK_0:
        break;
      case SDLK_m:
        m_uiManager.toggleDebugMenu();
        break;
      case SDLK_1:
        MapLayers::toggleLayer(Layer::TERRAIN);
        break;
      case SDLK_2:
        MapLayers::toggleLayer(Layer::BUILDINGS);
        break;
      case SDLK_3:
        MapLayers::toggleLayer(Layer::BLUEPRINT);
        break;
      case SDLK_4:
        MapLayers::toggleLayer(Layer::UNDERGROUND);
        break;
      case SDLK_i:
        m_tileInfoMode = !m_tileInfoMode;
        break;
      case SDLK_h:
        // TODO: This is only temporary until the new UI is ready. Remove this afterwards
        GameStates::instance().drawUI = !GameStates::instance().drawUI;
        break;
      case SDLK_f:
        engine.toggleFullScreen();
        break;
      case SDLK_w:
        if (Camera::cameraOffset.y > -2 * Settings::instance().screenHeight * Camera::zoomLevel)
        {
          // check if map exists to see, if we're ingame already.
          if (Engine::instance().map)
          {
            Camera::cameraOffset.y -= (Settings::instance().screenHeight / 16);
            // set the center coordinates for scrolling
            Camera::centerIsoCoordinates =
                convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
            Engine::instance().map->refresh();
          }
        }
        break;
      case SDLK_a:
        if (Camera::cameraOffset.x > -0.25 * Settings::instance().screenWidth * Camera::zoomLevel)
        {
          // check if map exists to see, if we're ingame already.
          if (Engine::instance().map)
          {
            Camera::cameraOffset.x -= (Settings::instance().screenWidth / 16);
            // set the center coordinates for scrolling
            Camera::centerIsoCoordinates =
                convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
            Engine::instance().map->refresh();
          }
        }
        break;

      case SDLK_s:
        if (Camera::cameraOffset.y < 1.25 * Settings::instance().screenHeight * Camera::zoomLevel)
        {
          // check if map exists to see, if we're ingame already.
          if (Engine::instance().map)
          {
            Camera::cameraOffset.y += (Settings::instance().screenHeight / 16);
            // set the center coordinates for scrolling
            Camera::centerIsoCoordinates =
                convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
            Engine::instance().map->refresh();
          }
        }
        break;

      case SDLK_d:
        if (Camera::cameraOffset.x < 5 * Settings::instance().screenWidth * Camera::zoomLevel)
        {
          // check if map exists to see, if we're ingame already.
          if (Engine::instance().map)
          {
            Camera::cameraOffset.x += (Settings::instance().screenWidth / 16);
            // set the center coordinates for scrolling
            Camera::centerIsoCoordinates =
                convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
            Engine::instance().map->refresh();
          }
        }
        break;

      default:
        break;
      }
      break;

    case SDL_MULTIGESTURE:
      if (event.mgesture.numFingers == 2)
      {
        m_panning = true;
        // check if we're pinching
        if (event.mgesture.dDist != 0)
        {
          // store pinchCenterCoords so they stay the same for all zoom levels
          if (pinchCenterCoords.x == 0 && pinchCenterCoords.y == 0)
          {
            pinchCenterCoords =
                convertScreenToIsoCoordinates({static_cast<int>(event.mgesture.x * Settings::instance().screenWidth),
                                               static_cast<int>(event.mgesture.y * Settings::instance().screenHeight)});
          }
          Camera::setPinchDistance(event.mgesture.dDist * 15.0F, pinchCenterCoords.x, pinchCenterCoords.y);
          m_skipLeftClick = true;
          break;
        }

        if (m_panning)
        {
          Camera::cameraOffset.x -= static_cast<int>(Settings::instance().screenWidth * event.tfinger.dx);
          Camera::cameraOffset.y -= static_cast<int>(Settings::instance().screenHeight * event.tfinger.dy);
          Camera::centerIsoCoordinates =
              convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
          Engine::instance().map->refresh();
          m_skipLeftClick = true;
          break;
        }
      }
      m_skipLeftClick = true;
      break;
    case SDL_MOUSEMOTION:
      // check for UI events first
      for (const auto &it : m_uiManager.getAllUiElements())
      {
        // if element isn't visible then don't event check it
        if (it->isVisible())
        {
          // spawn tooltip timer, if we're over an UI Element
          if (it->isMouseOver(event.button.x, event.button.y) && !it->getUiElementData().tooltipText.empty())
          {
            m_uiManager.startTooltip(event, it->getUiElementData().tooltipText);
          }
          // if the mouse cursor left an element, we're not hovering any more and we need to reset the pointer to null
          if (m_lastHoveredElement && !m_lastHoveredElement->isMouseOverHoverableArea(event.button.x, event.button.y))
          {
            // we're not hovering, so stop the tooltips
            m_uiManager.stopTooltip();
            // tell the previously hovered element we left it before resetting it
            m_lastHoveredElement->onMouseLeave(event);
            m_lastHoveredElement = nullptr;
            break;
          }
          // If we're over a UI element that has no click functionality, abort the event loop, so no clicks go through the UiElement.
          //Note: This is handled here because UIGroups have no dimensions, but are UiElements
          if (it->isMouseOverHoverableArea(event.button.x, event.button.y))
          {
            it->onMouseMove(event);
            // if the element we're hovering over is not the same as the stored "lastHoveredElement", update it
            if (it.get() != m_lastHoveredElement)
            {
              if (m_lastHoveredElement)
              {
                m_lastHoveredElement->onMouseLeave(event);
              }
              it->onMouseEnter(event);
              m_lastHoveredElement = it.get();
            }
            break;
          }
          // definitely figure out a better way to do this, this was done for the Slider
          if (it->isMouseOver(event.button.x, event.button.y))
          {
            it->onMouseMove(event);
          }
        }
      }

      //  Game Event Handling
      if (engine.map)
      {
        // clear highlighting
        unHighlightNodes();

        // if we're panning, move the camera and break
        if (m_panning)
        {
          Camera::cameraOffset.x -= event.motion.xrel;
          Camera::cameraOffset.y -= event.motion.yrel;

          if (Engine::instance().map != nullptr)
            Engine::instance().map->refresh();
          return;
        }
        // check if we should highlight tiles and if we're in placement mode
        else if (highlightSelection)
        {
          mouseScreenCoords = {event.button.x, event.button.y};
          mouseIsoCoords = convertScreenToIsoCoordinates(mouseScreenCoords);

          // if it's a multi-node tile, get the origin corner point
          Point origCornerPoint =
              engine.map->getNodeOrigCornerPoint(mouseIsoCoords, TileManager::instance().getTileLayer(tileToPlace));

          if (origCornerPoint == UNDEFINED_POINT)
          {
            origCornerPoint = mouseIsoCoords;
          }

          // if there's no tileToPlace use the current mouse coordinates
          if (tileToPlace.empty())
          {
            m_nodesToHighlight.push_back(mouseIsoCoords);
          }
          else
          {
            // get all node coordinates the tile we'll place occupies
            for (auto &node : engine.map->getObjectCoords(mouseIsoCoords, tileToPlace))
            {
              // if we don't geta correct coordinate, fall back to the click coordinates
              if (node == UNDEFINED_POINT && isPointWithinMapBoundaries(mouseIsoCoords))
              {
                m_nodesToHighlight.push_back(mouseIsoCoords);
              }
              else if (isPointWithinMapBoundaries(node))
              {
                m_nodesToHighlight.push_back(node);
              }
            }
          }

          // if mouse is held down, we need to check for plamentmodes LINE and RECTANGLE
          if ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)))
          {
            switch (GameStates::instance().placementMode)
            {
            case PlacementMode::SINGLE:
              m_nodesToPlace.push_back(mouseIsoCoords);
              break;
            case PlacementMode::LINE:
              m_nodesToPlace = createBresenhamLine(m_clickDownCoords, mouseIsoCoords);
              m_nodesToHighlight = m_nodesToPlace;
              break;
            case PlacementMode::RECTANGLE:
              m_nodesToPlace = getRectangleSelectionNodes(m_clickDownCoords, mouseIsoCoords);
              m_nodesToHighlight = m_nodesToPlace;
              break;
            }
          }

          // if we haven't any nodes to place yet, use the mouse coordinates
          if (m_nodesToPlace.empty())
          {
            m_nodesToPlace.push_back(mouseIsoCoords);
          }
          bool placementAllowed = false;

          // if we touch a bigger than 1x1 tile also add all nodes of the building to highlight.
          for (const auto &coords : m_nodesToHighlight)
          {
            Layer layer = TileManager::instance().getTileLayer(tileToPlace);
            Point currentOriginPoint = engine.map->getNodeOrigCornerPoint(coords, layer);
            std::string currentTileID = engine.map->getTileID(currentOriginPoint, layer);
            for (auto &foundNode : engine.map->getObjectCoords(currentOriginPoint, currentTileID))
            {
              // only add the node if it's unique
              if (std::find(m_nodesToHighlight.begin(), m_nodesToHighlight.end(), foundNode) == m_nodesToHighlight.end() && foundNode != UNDEFINED_POINT)
              {
                m_nodesToHighlight.push_back(foundNode);
              }
            }
          }
          // finally highlight all the tiles we've found
          for (const auto &highlitNode : m_nodesToHighlight)
          {
            if (!engine.map->isPlacementOnNodeAllowed(highlitNode, tileToPlace) || demolishMode)
            {
              // already occupied tile, mark red
              placementAllowed = false;
              break;
            }
            else
            {
              // mark gray.
              placementAllowed = true;
            }
          }
          for (const auto &highlitNode : m_nodesToHighlight)
          {
            if (placementAllowed)
            {
              engine.map->highlightNode(highlitNode, SpriteHighlightColor::GRAY);
            }
            else
            {
              engine.map->highlightNode(highlitNode, SpriteHighlightColor::RED);
            }
          }
        }
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      m_skipLeftClick = false;
      // check for UI events first
      for (const auto &it : m_uiManager.getAllUiElementsForEventHandling())
      {
        // only check visible elements
        if (it->isVisible() && it->onMouseButtonDown(event))
        {
          break;
        }
      }

      if (event.button.button == SDL_BUTTON_RIGHT)
      {
        m_panning = true;
      }
      else if (event.button.button == SDL_BUTTON_LEFT)
      {
        // game event handling
        mouseScreenCoords = {event.button.x, event.button.y};
        mouseIsoCoords = convertScreenToIsoCoordinates(mouseScreenCoords);

        if (isPointWithinMapBoundaries(mouseIsoCoords))
        {
          m_clickDownCoords = mouseIsoCoords;
        }
      }
      break;

    case SDL_MOUSEBUTTONUP:
    {
      std::vector<Point> bresenhamLineNodes = m_nodesToPlace;

      if (m_panning)
      {
        Camera::centerIsoCoordinates =
            convertScreenToIsoCoordinates({Settings::instance().screenWidth / 2, Settings::instance().screenHeight / 2});
        m_panning = false;
      }
      // reset pinchCenterCoords when fingers are released
      pinchCenterCoords = {0, 0, 0, 0};
      // check for UI events first
      for (const auto &it : m_uiManager.getAllUiElementsForEventHandling())
      {
        // only check visible elements
        if (it->isVisible() && event.button.button == SDL_BUTTON_LEFT)
        {
          // first, check if the element is a group and send the event
          if (it->onMouseButtonUp(event))
          {
            m_skipLeftClick = true;
            break;
          }
          // If we're over a UI element that has no click functionality, abort the event loop, so no clicks go through the UiElement.
          //Note: This is handled here because UIGroups have no dimensions, but are UiElements
          if (it->isMouseOver(event.button.x, event.button.y))
          {
            m_skipLeftClick = true;
          }
        }
      }
      // If we're over a ui element, don't handle game events
      if (m_skipLeftClick)
      {
        break;
      }

      // game event handling
      mouseScreenCoords = {event.button.x, event.button.y};
      mouseIsoCoords = convertScreenToIsoCoordinates(mouseScreenCoords);
      // gather all nodes the objects that'll be placed is going to occupy.
      std::vector targetObjectNodes = engine.map->getObjectCoords(mouseIsoCoords, tileToPlace);

      if (event.button.button == SDL_BUTTON_LEFT && isPointWithinMapBoundaries(targetObjectNodes))
      {
        if (m_tileInfoMode)
        {
          engine.map->getNodeInformation({mouseIsoCoords.x, mouseIsoCoords.y, 0, 0});
        }
        else if (terrainEditMode == TerrainEdit::RAISE)
        {
          engine.increaseHeight(mouseIsoCoords);
        }
        else if (terrainEditMode == TerrainEdit::LOWER)
        {
          engine.decreaseHeight(mouseIsoCoords);
        }
        else if (!tileToPlace.empty())
        {
          // if targetObject.size > 1 it is a tile bigger than 1x1
          if (targetObjectNodes.size() > 1)
          {
            // instead of using "nodesToPlace" which would be the origin-corner coordinate, we need to pass ALL occupied nodes for now.
            engine.setTileIDOfNode(targetObjectNodes.begin(), targetObjectNodes.end(), tileToPlace, false);
          }
          else
          {
            engine.setTileIDOfNode(m_nodesToPlace.begin(), m_nodesToPlace.end(), tileToPlace, true);
          }
        }
        else if (demolishMode)
        {
          engine.demolishNode(mouseIsoCoords);
        }
        else
        {
          LOG(LOG_INFO) << "CLICKED - Iso Coords: " << mouseIsoCoords.x << ", " << mouseIsoCoords.y;
        }
      }
      // when we're done, reset highlighting
      unHighlightNodes();
      break;
    }
    case SDL_MOUSEWHEEL:
      if (event.wheel.y > 0)
      {
        Camera::increaseZoomLevel();
      }
      else if (event.wheel.y < 0)
      {
        Camera::decreaseZoomLevel();
      }
      break;

    default:
      break;
    }
  }

  for (auto &timer : timers)
  {
    if (timer)
      timer->checkTimeout();
  }

  for (std::vector<Timer *>::iterator it = timers.begin(); it != timers.end();)
  {
    if (!(*it)->isActive())
    {
      it = timers.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void EventManager::registerTimer(Timer *timer) { timers.push_back(timer); }
