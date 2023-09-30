(function () {

"use strict";

var CORNERS_DEFAULT = 9,
    CORNERS_MIN = 3,
    CORNERS_MAX = 36,
    WIDTH = 500,
    HEIGHT = 500,
    POINT_SIZE = 15,
    CENTER = [WIDTH / 2, HEIGHT / 2],
    MAX_LENGTH = 10000,
    canvas,
    graph,
    walls,
    traces,
    corners_input,
    orientation_input,
    delays_input,
    resolution_input,
    lossb_input,
    lossd_input,
    scale_input,
    code_output,
    source,
    sink,
    delay_params,
    corners = [],
    paths = [],
    moving_point = null;

function main()
{
    var i;

    canvas = $("canvas");
    canvas.onmousemove = handle_canvas_mouse_move;

    graph = $("graph");
    walls = $("walls");
    traces = $("traces");

    corners_input = $("corners");
    corners_input.min = String(CORNERS_MIN);
    corners_input.max = String(CORNERS_MAX);
    corners_input.value = String(CORNERS_DEFAULT);
    corners_input.onchange = handle_corners_change;

    orientation_input = $("orientation");

    delays_input = $("delays");
    resolution_input = $("resolution");
    lossb_input = $("lossb");
    lossd_input = $("lossd");
    scale_input = $("scale");

    $("circle").onclick = handle_circle_click;
    $("trace").onclick = handle_trace_click;

    code_output = $("code");

    init_corners();

    source = new Point(-1, vsum(CENTER, [0, -90]), "source");
    sink = new Point(-1, vsum(CENTER, [30, 60]), "sink");
}

function init_corners()
{
    var i;

    for (i = 0; i != CORNERS_DEFAULT; ++i) {
        corners.push(new Point(i, vcopy(CENTER), "corner"));
    }

    arrange_corners_in_circle();
}

function arrange_corners_in_circle()
{
    var r = WIDTH * 0.8 / 2,
        angle = deg_to_rad(Number(orientation_input.value)),
        corner,
        i, l, p;

    for (i = 0, l = corners.length; i != l; ++i) {
        corner = corners[i];

        corner.pos = vsum(CENTER, vpol(angle, r));
        corner.update();

        angle += deg_to_rad(360 / l);
    }
}

function Point(idx, pos, class_name)
{
    var node = create_svg_node("ellipse"),
        size = String(POINT_SIZE);

    node.setAttribute("class", class_name);
    node.setAttribute("rx", size);
    node.setAttribute("ry", size);

    node.onmousedown = this.handle_mouse_down;
    node.onmouseup = this.handle_mouse_up;

    graph.appendChild(node);

    node.point_obj = this;

    this.idx = idx;
    this.pos = pos;
    this.node = node;

    this.update();
}

Point.prototype.update = function ()
{
    this.node.setAttribute("cx", String(this.pos[0]));
    this.node.setAttribute("cy", String(this.pos[1]));

    update_walls(this.idx);
};

Point.prototype.remove = function ()
{
    moving_point = null;

    graph.removeChild(this.node);
    update_walls(this.idx);
};

Point.prototype.handle_mouse_down = function (evt)
{
    moving_point = evt.target.point_obj;

    return stop_event(evt);
};

Point.prototype.handle_mouse_up = function (evt)
{
    moving_point = null;
};

function handle_canvas_mouse_move(evt)
{
    if (moving_point === null) {
        return true;
    }

    moving_point.pos = to_canvas_pos([evt.pageX, evt.pageY]);
    moving_point.update();
    clear_traces();

    return true;
}

function create_svg_node(node_name)
{
    return document.createElementNS("http://www.w3.org/2000/svg", node_name);
}

function update_walls(corner_idx)
{
    var p = [],
        i, l;

    l = corners.length;

    if (corner_idx < 0 || corner_idx > l) {
        return;
    }

    if (l < 1) {
        return;
    }

    for (i = 0; i < l; ++i) {
        p.push(corners[i].pos);
    }

    walls.setAttribute("d", path_to_svg(p, true));

    clear_traces();
}

function path_to_svg(path, closed)
{
    var d, i, l;

    l = path.length;

    if (l == 0) {
        return "";
    }

    d = "M" + vstr(path[0]);

    for (i = 1; i < l; ++i) {
        d += " L" + vstr(path[i]);
    }

    if (closed) {
        d += " L" + vstr(path[0]);
    }

    return d;
}

function clear_traces()
{
    while (paths.length > 0) {
        traces.removeChild(paths.pop());
    }
}

function trace_angles()
{
    var delays = Math.max(1, Number(delays_input.value) | 0),
        d_angle = Number(resolution_input.value),
        trace,
        results,
        angle,
        bounces,
        len,
        time,
        flip,
        loss_db,
        gain,
        lossb = Number(lossb_input.value),
        scale = Number(scale_input.value) / WIDTH,
        lossd = Number(lossd_input.value) * scale,
        code,
        i, l;

    clear_traces();

    results = [];

    for (angle = 0; angle < 360; angle += d_angle) {
        trace = trace_angle_back_from_sink_to_source(angle);

        if (trace !== null) {
            bounces = trace[1];
            len = trace[2];
            flip = trace[3];

            loss_db = bounces * lossb + len * lossd;
            gain = Math.pow(10, -loss_db / 20);
            time = len * scale / 343;

            results.push([time, gain, angle * flip, trace[0]]);
        }
    }

    results.sort(function (a, b) { return a[0] - b[0]; });

    delay_params = [];

    for (i = 0, l = Math.min(delays, results.length); i < l; ++i) {
        trace = results[i];
        time = trace[0];
        gain = trace[1];
        angle = trace[2];
        trace = trace[3];

        traces.appendChild(trace);
        paths.push(trace);
        delay_params.push(new DelayParam(time, gain, angle));
    }

    for (i = l; i < delays; ++i) {
        delay_params.push(new DelayParam(0, 0, 0));
    }

    code = ["            {"];

    for (i = 0, l = delay_params.length; i < l; ++i) {
        code.push([
            "                {",
                format_number(delay_params[i].time),
                ", ",
                format_number(delay_params[i].gain),
                ", ",
                format_number(angle_to_panning(delay_params[i].angle)),
            "},"
        ].join(""));
    }

    code.push("            },");

    code_output.value = code.join("\n");
}

function format_number(n)
{
    var d = 1000000;

    return String(Math.round(n * d) / d);
}

function angle_to_panning(angle)
{
    var abs = Math.abs(angle),
        s = angle < 0 ? -1 : 1;

    return s * Math.cos(deg_to_rad(angle));
}

function DelayParam(time, gain, angle)
{
    this.time = time;
    this.gain = gain;
    this.angle = angle;
}

function trace_angle_back_from_sink_to_source(angle)
{
    var path,
        path_len,
        segment,
        head,
        prev,
        prev_int_p,
        int_p,
        svg_path,
        is_bouncing,
        found_intersection,
        bounces,
        flip,
        t, t2, i, l, s;

    s = 1;
    is_bouncing = true;
    bounces = 0;
    angle = deg_to_rad(angle);
    path = [sink.pos];
    path_len = 0;
    head = [sink.pos, vsum(sink.pos, vpol(angle, 1))];
    prev = -1;
    int_p = sink.pos;
    flip = (vsub(source.pos, sink.pos)[0] * vsub(head[1], sink.pos)[0]) > 0 ? 1 : -1;

    while (is_bouncing && path_len < MAX_LENGTH) {
        found_intersection = false;

        for (i = 0, l = corners.length; i < l; ++i) {
            if (i === prev) {
                continue;
            }

            segment = [corners[i].pos, corners[(i + 1) % l].pos];
            t = find_intersection(segment, head);
            t2 = find_intersection(head, segment);

            if (t !== null && 0 <= t[0] && t[0] <= 1 && t2 !== null && t2[0] * s >= 0) {
                found_intersection = true;
                prev_int_p = int_p;
                int_p = vsum(segment[0], vscale(t[0], vsub(segment[1], segment[0])));

                if (path_len > 0 && pldistance(source.pos, [prev_int_p, int_p]) < POINT_SIZE) {
                    path_len += length([prev_int_p, int_p]);
                    is_bouncing = false;
                    path.push(source.pos);
                } else {
                    path.push(int_p);
                    path_len += length([prev_int_p, int_p]);
                    ++bounces;
                    angle += Math.PI - t[1] * 2;
                    head = [int_p, vsum(int_p, vpol(angle, 1))]
                    s *= -1;
                }
                prev = i;
                break;
            }
        }

        if (!found_intersection) {
            break;
        }
    }

    if (path_len >= MAX_LENGTH) {
        return null;
    }

    svg_path = create_svg_node("path");
    svg_path.setAttribute("d", path_to_svg(path));

    return [svg_path, bounces, path_len, flip];
}

function handle_corners_change(evt)
{
    var num_of_corners = Math.min(36, Math.max(3, Number(corners_input.value))) | 0,
        idx = corners.length,
        corner, corner_node,
        middle;

    corners_input.value = String(num_of_corners);

    while (num_of_corners > idx) {
        middle = vsum(vscale(0.5, corners[0].pos), vscale(0.5, corners[idx - 1].pos));
        corners.push(new Point(idx, middle, "corner"));

        ++idx;
    }

    while (num_of_corners < idx) {
        corners.pop().remove();

        --idx;
    }

    return true;
}

function handle_circle_click(evt)
{
    arrange_corners_in_circle();

    return stop_event(evt);
}

function handle_trace_click(evt)
{
    trace_angles();

    return stop_event(evt);
}

function $(obj)
{
    return (typeof(obj) == "string") ? document.getElementById(obj) : obj;
}

function stop_event(evt)
{
    evt = evt || event;
    evt.preventDefault();

    return false;
}

function to_canvas_pos(doc_pos)
{
    var rect = canvas.getClientRects()[0];

    return [
        (doc_pos[0] - rect.left) * WIDTH / rect.width,
        (doc_pos[1] - rect.top) * HEIGHT / rect.height
    ];
}

function vstr(vector)
{
    return String(vector[0]) + "," + String(vector[1]);
}

function vcopy(vector)
{
    return [vector[0], vector[1]];
}

function vpol(angle, radius)
{
    return [Math.cos(angle) * radius, Math.sin(angle) * radius];
}

function vscale(scalar, vector)
{
    return [scalar * vector[0], scalar * vector[1]];
}

function vmul(a, b)
{
    return [a[0] * b[0], a[1] * b[1]];
}

function vsum(a, b)
{
    return [a[0] + b[0], a[1] + b[1]];
}

function vsub(a, b)
{
    return [a[0] - b[0], a[1] - b[1]];
}

function find_intersection(segment, line)
{
    var x1 = segment[0][0], y1 = segment[0][1],
        x2 = segment[1][0], y2 = segment[1][1],
        x3 = line[0][0], y3 = line[0][1],
        x4 = line[1][0], y4 = line[1][1],
        d1 = (x1 - x3) * (y3 - y4),
        d2 = (y1 - y3) * (x3 - x4),
        d3 = (x1 - x2) * (y3 - y4),
        d4 = (y1 - y2) * (x3 - x4),
        d5 = (x2 - x1),
        d6 = (y2 - y1),
        d7 = (x4 - x3),
        d8 = (y4 - y3);

    if (d3 == d4) {
        return null;
    }

    return [(d1 - d2) / (d3 - d4), Math.atan2(d5 * d8 - d6 * d7, d5 * d7 + d6 * d8)];
}

function pldistance(point, line)
{
    var x1 = line[0][0], y1 = line[0][1],
        x2 = line[1][0], y2 = line[1][1],
        x0 = point[0], y0 = point[1];

    return Math.abs((x2 - x1) * (y1 - y0) - (x1 - x0) * (y2 - y1)) / length(line);
}

function length(segment)
{
    var d = vsub(segment[1], segment[0]);

    return Math.sqrt(d[0] * d[0] + d[1] * d[1]);
}

function rotate(point, origin, degrees)
{
    var rad = deg_to_rad(degrees),
        sin = Math.sin(rad),
        cos = Math.cos(rad),
        x, y;

    point = vsub(point, origin);
    x = point[0];
    y = point[1];

    return vsum(origin, [x * cos - y * sin, x * sin + y * cos]);
}

function deg_to_rad(deg)
{
    return deg * Math.PI / 180;
}

window.onload = main;

})();
