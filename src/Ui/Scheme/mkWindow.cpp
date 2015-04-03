//  Copyright (c) 2015 Hugo Amiardhugo.amiard@laposte.net
//  This software is provided 'as-is' under the zlib License, see the LICENSE.txt file.
//  This notice and the license may not be removed or altered from any source distribution.

#include <Ui/mkUiConfig.h>
#include <Ui/Scheme/mkWindow.h>

#include <Ui/Scheme/mkDockspace.h>

#include <Ui/Form/mkWidgets.h>

#include <Ui/Form/mkForm.h>
#include <Ui/Frame/mkInk.h>
#include <Ui/Frame/mkFrame.h>
#include <Ui/Frame/mkStripe.h>
#include <Ui/Frame/mkLayer.h>

#include <Ui/Widget/mkRootSheet.h>

#include <iostream>

using namespace std::placeholders;

namespace mk
{
	WindowHeader::WindowHeader(Window* window)
		: Sheet(styleCls())
		, mWindow(window)
		, mTooltip("Drag me")
	{
		mTitle = this->makeappend<Label>(mWindow->name());
		this->makeappend<Widget>(DivX::styleCls());
		mCloseButton = mWindow->closable() ? this->makeappend<CloseButton>(std::bind(&Window::close, mWindow)) : nullptr;
	}

	bool WindowHeader::leftDragStart(float xPos, float yPos)
	{
		UNUSED(xPos); UNUSED(yPos);
		if(mWindow->dock())
			mWindow->undock();

		mWindow->frame()->layer()->setOpacity(VOID);
		mWindow->frame()->layer()->moveToTop();
		return true;
	}

	bool WindowHeader::leftDrag(float xPos, float yPos, float xDif, float yDif)
	{
		UNUSED(xPos); UNUSED(yPos);
		if(mWindow->movable())
			mWindow->frame()->setPosition(mWindow->frame()->dposition(DIM_X) + xDif, mWindow->frame()->dposition(DIM_Y) + yDif);

		return true;
	}

	bool WindowHeader::leftDragEnd(float xPos, float yPos)
	{
		if(mWindow->dockable())
		{
			Widget* widget = this->rootSheet()->pinpoint(xPos, yPos);
			while(widget && widget->type() != Docksection::cls())
				widget = widget->parent();

			if(widget)
			{
				Docksection* section = widget->as<Docksection>()->docktarget(xPos, yPos);
				mWindow->dock(section);
			}
		}

		mWindow->frame()->layer()->setOpacity(OPAQUE);
		return true;
	}

	WindowSizer::WindowSizer(Window* window)
		: Sheet(styleCls())
		, mWindow(window)
		, mResizeLeft(false)
	{}

	bool WindowSizer::leftDragStart(float xPos, float yPos)
	{
		UNUSED(yPos);
		mWindow->frame()->as<Layer>()->moveToTop();
		if(xPos - mWindow->frame()->dabsolute(DIM_X) > mWindow->frame()->dsize(DIM_X) * 0.5f)
			mResizeLeft = false;
		else
			mResizeLeft = true;
		return true;
	}

	bool WindowSizer::leftDrag(float xPos, float yPos, float xDif, float yDif)
	{
		UNUSED(xPos); UNUSED(yPos);
		if(mResizeLeft)
		{
			mWindow->frame()->setPositionDim(DIM_X, mWindow->frame()->dposition(DIM_X) + xDif);
			mWindow->frame()->setSize(std::max(10.f, mWindow->frame()->dsize(DIM_X) - xDif), std::max(25.f, mWindow->frame()->dsize(DIM_Y) + yDif));
		}
		else
		{
			mWindow->frame()->setSize(std::max(10.f, mWindow->frame()->dsize(DIM_X) + xDif), std::max(25.f, mWindow->frame()->dsize(DIM_Y) + yDif));
		}
		return true;
	}

	bool WindowSizer::leftDragEnd(float xPos, float yPos)
	{
		UNUSED(xPos); UNUSED(yPos);
		return true;
	}

	WindowBody::WindowBody()
		: Sheet(styleCls())
	{}

	CloseButton::CloseButton(const Trigger& trigger)
		: Button("", styleCls(), trigger)
	{}

	Window::Window(const string& title, bool closable, bool dockable, const Trigger& onClose, Docksection* dock)
		: Sheet(dock ? DockWindow::styleCls() : Window::styleCls(), LAYER)
		, mName(title)
		, mClosable(closable)
		, mDockable(dockable)
		, mMovable(true)
		, mSizable(true)
		, mContent(nullptr)
		, mOnClose(onClose)
		, mDock(dock)
	{
		mFrame = make_unique<Layer>(this, 0);

		mHeader = this->makeappend<WindowHeader>(this);
		mBody = this->makeappend<WindowBody>();
		mFooter = this->makeappend<WindowSizer>(this);
	}

	Window::~Window()
	{}

	void Window::bind(Sheet* parent, size_t index)
	{
		Sheet::bind(parent, index);

		if(!mDock)
		{
			float x = this->rootSheet()->frame()->dsize(DIM_X) / 2 - mFrame->dsize(DIM_X) / 2;
			float y = this->rootSheet()->frame()->dsize(DIM_Y) / 2 - mFrame->dsize(DIM_Y) / 2;
			mFrame->setPosition(x, y);
		}
	}

	void Window::toggleClosable()
	{
		mHeader->closeButton()->frame()->visible() ? mHeader->closeButton()->hide() : mHeader->closeButton()->show();
	}

	void Window::toggleMovable()
	{
		mMovable = !mMovable;
	}

	void Window::toggleResizable()
	{
		mSizable = !mSizable;
		mSizable ? mFooter->show() : mFooter->hide();
	}

	void Window::showTitlebar()
	{
		mHeader->show();
	}

	void Window::hideTitlebar()
	{
		mHeader->hide();
	}

	const string& Window::name()
	{
		return mContent ? mContent->name() : mName;
	}

	void Window::dock(Docksection* docksection)
	{
		std::cerr << ">>>>>>>>>>>  Window :: dock" << std::endl;
		this->docked();
		mDock = docksection;
		docksection->dock(this);
	}

	void Window::docked()
	{
		this->reset(DockWindow::styleCls());
		if(mSizable)
			mFooter->hide();
	}

	void Window::undock()
	{
		std::cerr << ">>>>>>>>>>>  Window :: undock" << std::endl;
		mDock->undock(this);
		mDock = nullptr;
		this->undocked();
	}

	void Window::undocked()
	{
		this->reset(Window::styleCls());
		if(mSizable)
			mFooter->show();

		mFrame->setPosition(mFrame->dabsolute(DIM_X), mFrame->dabsolute(DIM_Y));
		mFrame->as<Layer>()->moveToTop();
	}
	
	void Window::close()
	{
		if(mOnClose)
			mOnClose(this);
		this->destroy();
	}

	Widget* Window::vappend(unique_ptr<Widget> widget)
	{
		mHeader->title()->setLabel(widget->name());
		mContent = widget.get();
		return mBody->append(std::move(widget));
	}

	bool Window::leftClick(float x, float y)
	{
		UNUSED(x); UNUSED(y);
		if(!mDock)
			mFrame->as<Layer>()->moveToTop();
		return true;
	}

	bool Window::rightClick(float x, float y)
	{
		UNUSED(x); UNUSED(y);
		if(!mDock)
			mFrame->as<Layer>()->moveToTop();
		return true;
	}
}
