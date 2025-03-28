/*
 * Copyright (c) 2025, Tim Flynn <trflynn89@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/JsonObject.h>
#include <LibDevTools/Actors/TabActor.h>
#include <LibDevTools/Actors/WatcherActor.h>
#include <LibDevTools/DevToolsDelegate.h>
#include <LibDevTools/DevToolsServer.h>

namespace DevTools {

NonnullRefPtr<TabActor> TabActor::create(DevToolsServer& devtools, String name, TabDescription description)
{
    return adopt_ref(*new TabActor(devtools, move(name), move(description)));
}

TabActor::TabActor(DevToolsServer& devtools, String name, TabDescription description)
    : Actor(devtools, move(name))
    , m_description(move(description))
{
}

TabActor::~TabActor()
{
    reset_selected_node();
}

void TabActor::handle_message(Message const& message)
{
    JsonObject response;

    if (message.type == "getFavicon"sv) {
        // FIXME: Firefox DevTools wants a favicon URL here, but supplying a URL seems to prevent this tab from being
        //        listed on the about:debugging page. Both Servo and Firefox itself supply `null` here.
        response.set("favicon"sv, JsonValue {});
        send_response(message, move(response));
        return;
    }

    if (message.type == "getWatcher"sv) {
        if (!m_watcher)
            m_watcher = devtools().register_actor<WatcherActor>(this);

        response.set("actor"sv, m_watcher->name());
        response.set("traits"sv, m_watcher->serialize_description());
        send_response(message, move(response));
        return;
    }

    send_unrecognized_packet_type_error(message);
}

JsonObject TabActor::serialize_description() const
{
    JsonObject traits;
    traits.set("watcher"sv, true);
    traits.set("supportsReloadDescriptor"sv, true);

    // FIXME: We are using the tab's ID multiple times here. This is likely not correct, as both Firefox and Servo
    //        provide different IDs for browserId, browsingContextID, and outerWindowID.
    JsonObject description;
    description.set("actor"sv, name());
    description.set("title"sv, m_description.title);
    description.set("url"sv, m_description.url);
    description.set("browserId"sv, m_description.id);
    description.set("browsingContextID"sv, m_description.id);
    description.set("outerWindowID"sv, m_description.id);
    description.set("traits"sv, move(traits));
    return description;
}

void TabActor::reset_selected_node()
{
    devtools().delegate().clear_highlighted_dom_node(description());
    devtools().delegate().clear_inspected_dom_node(description());
}

}
