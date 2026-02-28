use <utils.scad>;
use <screws.scad>;

// constants
$e = 0.01; // small extra for removing artifacts during differencing
$2e = 2 * $e; // twice epsilon for extra length when translating by -e
$fn = $preview ? 10 : 60; // number of default vertices when rendering cylinders
nylon_tol = 0.1; // extra tolerance for carbon fibre nylon parts shrinkage
tglase_tol = 0.025; // extra tolerance for tglase parts

/** device general settings **/

$extra_tolerance = 2 * nylon_tol; // for cutouts, screws, etc.
//$extra_tolerance = 2 * tglase_tol; // for cutouts, screws, etc.
$extra_tolerance_2D = [$extra_tolerance, $extra_tolerance]; // tolerance in 2 d
$screw_hole_tolerance = 0.45 + $extra_tolerance; // tolerance for screw holes
$screw_threadable_tolerance = 0; // tolerance for screw holes that can be threaded into
$screw_vertical_stretch = 0.5; // vertical stretch
$layer_height = 0.25;
$spokes = 0.05; // width of print enhancement spokes
$nozzle = 0.4; // nozzle width for spacing decisions

// holder overall sizes
$holder_inner_diameter = 62; // fix inner diameter for adapter compability
$holder_inner_radius = $holder_inner_diameter/2;
$wall = 4 * $nozzle; // thickness of all walls
$holder_bottom_thickness = 2.5; // thickness of base
$holder_base_height = 14;
$holder_sensors_height = 14;
$holder_total_height = $holder_bottom_thickness + $holder_base_height + $holder_sensors_height;
$holder_feet_screw_type = "6-32"; // what screw type for the feet
$holder_feet_screw_thread_height = 6.35; // 1/4" length
$holder_feet_screw_wall = $wall; // thickness of the wall around the feet threads
$holder_feet_location = [54, 180, 306]; // angle locations
$holder_gap_width = 4; // gap for bottle view
$holder_feet_diameter = get_screw($holder_feet_screw_type)[1] + 2 * $holder_feet_screw_wall;
$holder_vent_diameter = $holder_inner_diameter - 15;
$holder_vent_width = 2.5;
$holder_notch_width = 2.5;
$holder_notch_length = 8;
$holder_notch_position = $holder_vent_diameter/2 + $holder_vent_width/2;

// stirrer
$stirrer_hole_diameter = 29; // diameter of the center hole for the magnetic stirrer (fits the largest stirrer at ~27mm)
$stirrer_screw_type = "M4"; // what type of screw type for stirrer attachment
$stirrer_screw_distance = 52; // distance between the screws
$stirrer_screw_rotation = 135; // how much to rotate (counter clockwise)

// sensor and beam
$block_attachment_screws_diameter = get_screw("M3")[1] - 0.25; // screws diameter
$beam_position_z = 7; // from top
$light_tunnel_height = 4.0; // height of the light tunnel (photodiode area cross section ~ 3.35mm)
$sensor_light_tunnel_width = 2.75; // width of the sensor light tunnel (photodiode area cross section ~ 3.35mm)
$sensor_block_roof = 3 * $layer_height;
$sensor_block_floor = 2 * $layer_height; // 1 layer at the bottom
$sensor_block_roof_min = 1 * $layer_height; // min height of the sloping roof (at outer edge)
$sensor_block_roof_angle = 25; // angle of roof [in degrees]
$sensor_block_screws_x = 13; // +/- from center
$sensor_block_screws_z = [$beam_position_z, 26]; // from top
$sensor_block_depth = 8.5;
$sensor_block_total_depth = $holder_inner_radius + $wall + $sensor_block_depth;
$sensor_block_outer_width = 2 * $sensor_block_screws_x + $spokes + 2 * $wall;
$sensor_block_inner_width = 2 * $sensor_block_screws_x - $spokes - 2 * $wall;
$sensor_columns_width = 11.0; // columns left/right of sensors
$sensor_columns_start = 15.0; // where the columns start
$beam_light_tunnel_width = 0.8; // width of light tunnel (same hight as sensor block)
$beam_block_roof = 3 * $layer_height; // $sensor_block_floor;
$beam_block_roof_min = $sensor_block_roof_min;
$beam_block_roof_angle = $sensor_block_roof_angle;
$beam_block_screws_x = 8; // +/- from center
$beam_block_screws_z = $beam_position_z; // from top
$beam_block_height = 14; // from top
$beam_block_depth = 5;
$beam_block_total_depth = $holder_inner_radius + $wall + $beam_block_depth;
$beam_block_outer_width = 2 * $beam_block_screws_x + $spokes + 2 * $wall;
$beam_block_inner_width = 2 * $beam_block_screws_x - $spokes - 2 * $wall;

// ir temp sensor
$temp_position_z = 17; // from top
$temp_tunnel_diameter = 4.0; // diameter of sensor entrance

/** micrologger device **/

// FIXME: use the spokes distribution functions used in the base adapters
module device(spokes = true) {

  difference() {

    union() {
      // ring
      with_base_magnets(top = $holder_total_height, spokes = spokes)
        cylinder(d = $holder_inner_diameter + 2 * $wall, h = $holder_total_height);

      // feet threads
      for (angle = $holder_feet_location) {
        translate([$holder_inner_radius * cos(angle), $holder_inner_radius * sin(angle), 0])
        rotate([0, 0, 90 + angle])
        translate([0, -$holder_feet_diameter/2 - 1, 0])
        difference() {
          // foot
          union() {
            // thread core
            linear_extrude($holder_bottom_thickness)
              translate([-$holder_feet_diameter/2, 0, 0])
                square([$holder_feet_diameter, $holder_feet_diameter/2]);
            linear_extrude($holder_feet_screw_thread_height)
              circle(d = $holder_feet_diameter);
            // smooth edges to ring for bottom
            linear_extrude($holder_bottom_thickness)
            for (x = [0, 1]) {
              mirror([x, 0, 0])
                translate([$holder_feet_diameter/2, 0, 0])
                  difference() {
                    square($holder_feet_diameter/2);
                    translate([$holder_feet_diameter/2, 0, 0]) circle(d = $holder_feet_diameter);
                  }
            }
          }

          // foot thread hole
          translate([0, 0, -$e])
            cylinder(d = get_screw($holder_feet_screw_type)[1] + $screw_threadable_tolerance, h = $holder_feet_screw_thread_height + $2e);

          // spokes
          if (spokes) {
            translate([-$spokes/2, -$holder_feet_diameter, -$e])
              cube([$spokes, $holder_feet_diameter, $holder_bottom_thickness + $e], center = false);
          }
        }
      }

      // beam block
      rotate([0, 0, 180])
      difference() {
        union() {
            difference() {
              // block
              translate([-$beam_block_outer_width/2, 0, 0])
                cube([$beam_block_outer_width, $beam_block_total_depth, $holder_total_height], center = false);
              // cutout
              translate([-$beam_block_inner_width/2, 0, 0])
                cube([$beam_block_inner_width, $beam_block_total_depth + $e, $holder_total_height - $beam_block_roof], center = false);
              // spokes
              if (spokes) {
                spoke = [$beam_block_outer_width, $spokes, $beam_block_roof - $layer_height];
                translate([-$beam_block_outer_width/2 + $wall/2, $beam_block_total_depth - $beam_block_depth/2, $holder_total_height - $beam_block_roof])
                distribute_along_y(
                  spoke, length = $beam_block_depth, pad_left = 0
                );
                // last layer
                /*
                translate([0, 0, $holder_total_height - $layer_height])
                distribute_along_x(
                  size = [$spokes, $beam_block_total_depth, $layer_height + $e],
                  length = $beam_block_outer_width
                );
                */
              }
              // sloped supports
              rotate([0, -90, 0])
                translate([0, 0, -$beam_block_outer_width/2 - $e])
                linear_extrude($beam_block_outer_width + $2e)
                polygon([
                  [-$e, $holder_total_height - $beam_block_height],
                  [$holder_total_height - $beam_block_height, $beam_block_total_depth + $e],
                  [-$e, $beam_block_total_depth + $e]
                ]);
              // support spokes
              if (spokes) {
                for (x = [-1, 1])
                  translate([-$spokes/2 + x * $beam_block_screws_x, 0, 0])
                    cube([$spokes, $beam_block_total_depth + $2e, $holder_total_height - $beam_block_height], center = false);
              }
            }
            // pcb attachment screw walls
            for (z = $beam_block_screws_z)
              for (x = [-1, 1])
                for (offset = [-1, 1])
                  translate([x * $beam_block_screws_x + offset * $wall, 0, $holder_total_height - z])
                    rotate([-90, 0, 0])
                      resize([0, $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance + 2 * $screw_vertical_stretch, 0])
                        cylinder(d = $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance, h = $beam_block_total_depth);
        }
        // pcb attachment screws
        for (z = $beam_block_screws_z)
          for (x = [-1, 1])
            translate([x * $beam_block_screws_x, -$e, $holder_total_height - z])
                rotate([-90, 0, 0])
                  resize([0, $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance + 2 * $screw_vertical_stretch, 0])
                    cylinder(d = $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance, h = $beam_block_total_depth + $2e);
      }

      // sensor block
      difference() {
          union() {
              difference() {
                // block
                translate([-$sensor_block_outer_width/2, 0, 0])
                  cube([$sensor_block_outer_width, $sensor_block_total_depth, $holder_total_height], center = false);
                // cutouts
                translate([-$sensor_block_inner_width/2, 0, $sensor_block_floor])
                  cube([$sensor_block_inner_width, $sensor_block_total_depth + $e, $holder_total_height - $sensor_block_roof - $sensor_block_floor], center = false);
                // roof spokes
                if (spokes) {
                  translate([-$sensor_block_outer_width/2 + $wall/2, $sensor_block_total_depth - $sensor_block_depth/2, $holder_total_height - $sensor_block_roof])
                  distribute_along_y(
                    size = [$sensor_block_outer_width, $spokes, $sensor_block_roof - $layer_height],
                    length = $sensor_block_depth, pad_left = 0
                  );
                  translate([-$sensor_block_outer_width/2 + $e, ($sensor_block_total_depth - $sensor_block_depth)/2, $holder_total_height - $sensor_block_roof])
                  distribute_along_y(
                    size = [$sensor_block_outer_width + $2e, $spokes, $sensor_block_roof - $layer_height],
                    length = $sensor_block_total_depth - $sensor_block_depth
                  );
                  // last layer
                  /*
                  translate([0, 0, $holder_total_height - $layer_height])
                  distribute_along_x(
                    size = [$spokes, $sensor_block_total_depth, $layer_height + $e],
                    length = $sensor_block_outer_width
                  );
                  */
                }

                /*
                translate([-$sensor_block_outer_width/2, 0, 0])
                  // block
                  cube([$sensor_block_outer_width, $sensor_block_total_depth, $holder_total_height], center = false);
                  // cutout with sloped roof
                  sensor_block_roof_max = tan ($sensor_block_roof_angle) * $sensor_block_total_depth;
                  translate([0, 0, $sensor_block_floor])
                    rotate([0, -90, 0])
                    translate([0, 0, -$sensor_block_inner_width/2])
                    linear_extrude($sensor_block_inner_width)
                    polygon([
                      [0, -$e],
                      [$holder_total_height - sensor_block_roof_max - $sensor_block_roof_min - $sensor_block_floor, - $e],
                      [$holder_total_height - $sensor_block_roof_min - $sensor_block_floor, $sensor_block_total_depth + $e],
                      [0, $sensor_block_total_depth + $e]
                    ]);
                  // roof spokes
                  if (spokes) {
                    n_spokes = round($sensor_block_inner_width/2.0);
                    for (i = [0:1:n_spokes]) {
                      translate([
                          -$sensor_block_inner_width/2 + i * ($sensor_block_inner_width - $spokes)/n_spokes,
                          0,
                          $sensor_block_floor])
                        cube([$spokes, $sensor_block_total_depth + $2e, $holder_total_height - 2 * $sensor_block_floor], center = false);
                    }
                  }
                  */
              }

              // sensor columns
              difference() {
                // block
                translate([-$sensor_columns_width/2 - $wall/2, 0, 0])
                  cube([$sensor_columns_width + $wall, $sensor_block_total_depth, $holder_total_height - $sensor_block_roof], center = false);
                // cutouts
                translate([-$sensor_columns_width/2, 0, $sensor_block_floor])
                  cube([$sensor_columns_width, $sensor_block_total_depth + $e, $holder_total_height - $sensor_block_roof - $sensor_block_floor], center = false);
                // cut out lower part of columns
                rotate([0, -90, 0])
                  translate([0, 0, -$sensor_columns_width/2 - $wall/2 - $e])
                  linear_extrude($sensor_columns_width + $wall + $2e)
                  polygon([
                    [-$e, $holder_inner_radius],
                    [$sensor_columns_start, $sensor_block_total_depth + $e],
                    [-$e, $sensor_block_total_depth + $e]
                  ]);
              }

              // pcb attachment screw walls
              for (z = $sensor_block_screws_z)
                for (x = [-1, 1])
                  for (offset = [-1, 1])
                    translate([x * $sensor_block_screws_x + offset * $wall, 0, $holder_total_height - z])
                      rotate([-90, 0, 0])
                        resize([0, $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance + 2 * $screw_vertical_stretch, 0])
                          cylinder(d = $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance, h = $sensor_block_total_depth);
          }
          // pcb attachment screws
          for (z = $sensor_block_screws_z)
            for (x = [-1, 1])
              translate([x * $sensor_block_screws_x, -$e, $holder_total_height - z])
                  rotate([-90, 0, 0])
                    resize([0, $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance + 2 * $screw_vertical_stretch, 0])
                      cylinder(d = $block_attachment_screws_diameter + 2 * $screw_threadable_tolerance, h = $sensor_block_total_depth + $2e);
      }
    }

    // sensor block beam
    translate([0, $sensor_block_total_depth/2, $holder_total_height - $beam_position_z])
      cube([$sensor_light_tunnel_width, $sensor_block_total_depth, $light_tunnel_height], center = true);

    // beam block beam
    translate([0, -$beam_block_total_depth/2, $holder_total_height - $beam_position_z])
      cube([$beam_light_tunnel_width, $beam_block_total_depth, $light_tunnel_height], center = true);

    // beam block ir sensor
    translate([0, 0, $holder_total_height - $temp_position_z])
      rotate([-90, 0, 0])
        cylinder(d = $temp_tunnel_diameter, h = $sensor_block_total_depth + $e);

    // spokes
    if (spokes) {
      // spokes for sensor block attachment screws
      difference() {
        for (x1 = [-1, 1]) {
          translate([
              -$spokes/2 + x1 * $sensor_block_screws_x,
              0,
              $holder_bottom_thickness])
            cube([$spokes, $sensor_block_total_depth - $wall, $holder_total_height - $holder_bottom_thickness - $sensor_block_floor - $sensor_block_roof], center = false);
        }
        translate([0, $sensor_block_total_depth/2, $holder_total_height - $sensor_block_screws_z[0] - $block_attachment_screws_diameter/2 - $screw_vertical_stretch])
          xy_center_cube([$sensor_block_outer_width, 2 * $sensor_block_total_depth, $block_attachment_screws_diameter + 2 * $screw_vertical_stretch]);
        translate([0, $sensor_block_total_depth/2, $holder_total_height - $sensor_block_screws_z[1] - $block_attachment_screws_diameter/2 - $screw_vertical_stretch])
          xy_center_cube([$sensor_block_outer_width, 2 * $sensor_block_total_depth, $block_attachment_screws_diameter + 2 * $screw_vertical_stretch]);
      }
      // spokes for beam block attachment screws
      rotate([0, 0, 180])
      difference() {
        for (x1 = [-1, 1]) {
          translate([
              -$spokes/2 + x1 * $beam_block_screws_x,
              0,
              $holder_total_height - $beam_block_height])
            cube([$spokes, $beam_block_total_depth - $wall, $beam_block_height - $beam_block_roof], center = false);
        }
        translate([0, $beam_block_total_depth/2, $holder_total_height - $beam_block_screws_z - $block_attachment_screws_diameter/2 - $screw_vertical_stretch])
          xy_center_cube([$beam_block_outer_width, 2 * $beam_block_total_depth, $block_attachment_screws_diameter + 2 * $screw_vertical_stretch]);
      }
      // spokes for stirrer cutout
      translate([$holder_vent_diameter/2, $spokes/2, -$e])
        rotate([0, 0, 180])
          cube([$holder_inner_radius + $holder_vent_diameter/2 + $wall, $spokes, $holder_feet_screw_thread_height + $e], center = false);
      // spokes for motor attachment screws
      rotate([0, 0, $stirrer_screw_rotation])
      translate([0, 0, -$e])
        xy_center_cube([$stirrer_screw_distance, $spokes, $holder_bottom_thickness + $2e]);
      // spokes for tri-supports
      for (angle = $holder_feet_location)
        rotate([0, 0, -90 + angle])
          translate([-$spokes/2, 0, $holder_bottom_thickness])
            cube([$spokes, $holder_inner_radius + $holder_feet_diameter/2, $holder_feet_screw_thread_height - $holder_bottom_thickness + $e], center = false);
    }

    // gap below ir tunnel
    translate([0, -$holder_gap_width/2, $holder_bottom_thickness])
      cube([$holder_inner_radius + $wall + $e, $holder_gap_width, $holder_total_height - $temp_position_z - $temp_tunnel_diameter/2 - $holder_bottom_thickness], center = false);

    // gap between ir tunnel and light tunnel
    translate([0, -$holder_gap_width/2, $holder_total_height - $temp_position_z + $temp_tunnel_diameter/2])
      cube([$holder_inner_radius + $wall + $e, $holder_gap_width, $temp_position_z - $beam_position_z - $temp_tunnel_diameter/2 - $light_tunnel_height/2], center = false);

    // gap from light tunnel to top
    translate([0, -$holder_gap_width/2, $holder_total_height - $beam_position_z + $light_tunnel_height/2])
      cube([$holder_inner_radius + $wall + $e, $holder_gap_width, $holder_total_height - $beam_position_z - $light_tunnel_height/2 + $e], center = false);

    // center cutout
    translate([0, 0, $holder_bottom_thickness])
      cylinder(d = $holder_inner_diameter, h = $holder_total_height);

    // stirrer cutout
    translate([0, 0, -$e])
      cylinder(d = $stirrer_hole_diameter, h = $holder_bottom_thickness + $2e);

    // vent cutouts
    difference() {
      translate([0, 0, -$e])
        cylinder(d = $holder_vent_diameter + 2 * $holder_vent_width, h = $holder_bottom_thickness + $2e);
      translate([0, 0, -$2e])
        cylinder(d = $holder_vent_diameter, h = $holder_bottom_thickness + 2 * $2e);
      for (i = [0, 1])
        translate([(i - 1) * $holder_inner_radius, -i * $holder_inner_radius, -$2e])
          cube([$holder_inner_radius, $holder_inner_radius, $holder_bottom_thickness + 2 * $2e], center = false);
    }

    // notch cutout
    for (y = [-1, 1])
    translate([-$holder_notch_width/2, -$holder_notch_length/2 + y * $holder_notch_position, -$e])
    cube([$holder_notch_width, $holder_notch_length, $holder_bottom_thickness + $2e]);

    // stirrer attachment screws
    for (x = [-1, 1]) {
      translate([x * $stirrer_screw_distance/2 * cos($stirrer_screw_rotation), x * $stirrer_screw_distance/2 * sin($stirrer_screw_rotation), -$e])
        machine_screw($stirrer_screw_type, $holder_bottom_thickness + $2e, invert_countersink = true, tolerance = $screw_hole_tolerance);
    }

  }

}

/** base/adapter attachment magnets **/

// magnet locations
//$magnet_location = [90 + 42, 90 - 42, 270 + 42, 270 - 42];
$magnet_location = [90 + 36, 90 - 36, 270 + 36, 270 - 36];
$magnet_size = [7, 3.25, 7] + [0.25, 0.25, 0.5]; // actual magnet size (depth, width, height) + buffers

// adapter magnets
$magnet_adapter_magnet_base = 2 * $layer_height; // base of adapter
$magnet_adapter_magnet_roof = 2 * $layer_height; // top of adapter
$magnet_adapter_height = $magnet_size[2] + $magnet_adapter_magnet_base + $magnet_adapter_magnet_roof;
$magnet_adapter_width = $magnet_size[1] + 2 * $wall; // 7.5 mm
$magnet_adapter_depth = $holder_inner_radius + 2.5 * $wall + $magnet_size[0];
$magnet_adapter_lip = 2.5; // how far below the rim of the base does the adapter start
$magnet_adapter_min_gap = 1.0; // minimum gap between holder ring and adapter

// adapter ridge ridge
$magnet_base_ridge_depth = $magnet_adapter_depth - $wall - $magnet_size[0];
$magnet_base_ridge_width = $magnet_adapter_width - 2 * $wall;
$magnet_adapter_ridge_depth = $magnet_base_ridge_depth - 0.5;
$magnet_adapter_ridge_width = $magnet_base_ridge_width - 0.75;
$magnet_adapter_connector_width = 2 * $wall;

// base magnets
$magnet_base_magnet_base = 2 * $layer_height; // base of magnet in base
$magnet_base_magnet_roof = 2 * $layer_height; // top of magnet in base
$magnet_base_total_height = $magnet_size[2] + $magnet_base_magnet_base + $magnet_base_magnet_roof;
$magnet_base_width = $magnet_adapter_width;
$magnet_base_depth = $magnet_adapter_depth;

// base holder mangets
// @param top z-coordinate of the top of the magnets
module with_base_magnets(top, spokes = true, ridges = true, last_layer = false) {
  // indices for each magnet holder
  indices = [0:1:(len($magnet_location) - 1)];
  difference() {
    // UNION START
    union() {
      children(0);
      // magnet holder supports
      for (i = indices) {
        rotate([0, 0, -90 + $magnet_location[i]])
          translate([-$magnet_base_width/2, 0, 0])
            // support block
            difference() {
              cube([$magnet_base_width, $magnet_base_depth, top - $magnet_base_total_height], center = false);
              // supports underhang cutout
              translate([$magnet_base_width + $e, 0, -$e])
                rotate([0, -90, 0])
                  linear_extrude($magnet_base_width + $2e)
                  // FIXME: should be calculate based on 45 degree angle instead of guess
                    polygon([
                      [-20, -$e],
                      [-20, $magnet_base_depth + $e],
                      [top - $magnet_base_total_height, $magnet_base_depth + $e],
                      [-20, -$e]
                    ]);
            }
      }
      // magnet chamber
      for (i = indices) {
        rotate([0, 0, -90 + $magnet_location[i]])
          difference() {
            // housing
            translate([-$magnet_base_width/2, 0, top  - $magnet_base_total_height])
              cube([$magnet_base_width, $magnet_base_depth, $magnet_base_total_height], center = false);
            // magnet
            translate([-$magnet_size[1]/2, $magnet_base_depth - $magnet_size[0], top  - $magnet_base_total_height + $magnet_base_magnet_base])
              cube([$magnet_size[1], $magnet_size[0] + $e, $magnet_size[2]], center = false);
          }
      }
    }
    // UNION END

    // DIFFERENCE START
    // ridge cutouts
    if (ridges) {
      for (i = indices) {
        rotate([0, 0, -90 + $magnet_location[i]])
          translate([-$magnet_base_ridge_width/2, 0, top - $magnet_base_total_height + $magnet_base_magnet_base])
            cube([$magnet_base_ridge_width, $magnet_base_ridge_depth, $magnet_base_total_height - $magnet_base_magnet_base + $e], center = false);
      }
    }

    // spokes
    if (spokes) {
      // spokes for magnet holder supports + base
      difference() {
        for (i = indices) {
          rotate([0, 0, -90 + $magnet_location[i]])
            distribute_along_x(
              size = [$spokes, $magnet_base_depth + $e, top  - $magnet_base_total_height + $magnet_base_magnet_base],
              length = $magnet_base_width
            );
        }
        children(0);
      }
      // spokes for magnet roof
      for (i = indices) {
        rotate([0, 0, -90 + $magnet_location[i]]) {
          // lower layers
          translate([-$magnet_base_width/2, $magnet_base_depth - $magnet_size[0], top  - $magnet_base_magnet_roof])
              distribute_along_y(
                size = [$magnet_base_width, $spokes, $magnet_base_magnet_roof - $layer_height],
                length = $magnet_size[0],
                pad_left = 0, align = 0,
                offset = $wall/2, alternate = true
              );
          // last layer
          if (last_layer) {
            translate([0, $magnet_base_depth - $magnet_size[0], top - $layer_height])
                distribute_along_x(
                  size = [$spokes, $magnet_size[0], $layer_height + $e],
                  length = $magnet_adapter_width
                );
          }
        }
      }
    }
    // DIFFERENCE END
  }
}

// adapter magnets
module with_adapter_magnets(inside_diameter = 0, spokes = true, base_insert = true, ridges = true, sensors = false) {
  indices = [0:1:(len($magnet_location) - 1)];
  difference() {
    // UNION START
    union() {
      // main ring
      difference() {
        children(0);
        // minus inner ring above the base
        if (base_insert)
          translate([0, 0, $magnet_adapter_height])
            difference() {
              cylinder(d = 2 * $holder_inner_radius + 2 * $wall + $e, h = 2 * $holder_total_height + $e);
              translate([0, 0, -$e])
                cylinder(d = 2 * $holder_inner_radius - $magnet_adapter_min_gap, h = 2 * $holder_total_height + 3 * $e);
            }
      }
      // magnet holders
      difference() {
        for (i = indices) {
          rotate([0, 0, -90 + $magnet_location[i]])
          // magnet chamber
          difference() {
            union() {
              // magnet chamber
              translate([-$magnet_adapter_width/2, $holder_inner_radius, 0])
                cube([$magnet_adapter_width, $magnet_adapter_depth - $holder_inner_radius, $magnet_adapter_height], center = false);
              // connectors
              translate([-$magnet_adapter_connector_width/2, 0, 0])
                cube([$magnet_adapter_connector_width, $magnet_adapter_depth, $magnet_adapter_height], center = false);
              // ridges
              if (ridges)
                translate([-$magnet_adapter_ridge_width/2, 0, $magnet_adapter_height])
                  cube([$magnet_adapter_ridge_width, $magnet_adapter_ridge_depth, $magnet_adapter_lip], center = false);
              // sensors chambers
              if (sensors) {
                translate([-(2 * $wall + $sensor_setscrew_diameter)/2, 0, 0])
                  cube([2 * $wall + $sensor_setscrew_diameter, inside_diameter/2 + $wall + $sensor_thickness, $sensor_start + $sensor_roof], center = false);
              }
            }
            // spokes
            if (spokes) {
              for (z = [0, $magnet_adapter_height - 2 * $layer_height]) {
                // chamber base spokes (1st layer) - along y axis
                translate([-$magnet_adapter_width/2, $holder_inner_radius, z])
                  distribute_along_y(
                    size = [$magnet_adapter_width, $spokes, $layer_height],
                    length = $magnet_adapter_depth - $holder_inner_radius,
                    align = 0, pad_left = $wall, offset = $wall/2, alternate = true
                  );
                // chamber base spokes (2nd layer) - along x axis
                translate([0, $holder_inner_radius + $wall, z + $layer_height])
                  distribute_along_x(
                    size = [$spokes, $magnet_adapter_depth - $holder_inner_radius - $wall + $e, $layer_height],
                    length = $magnet_adapter_width
                  );
              }
              // connector spokes for base
              translate([-$spokes/2, 0, -$e])
                cube([$spokes, $holder_inner_radius + $wall, $magnet_adapter_magnet_base + $e], center = false);
              // connector spokes between magnet chamber base and roof
              translate([-$spokes/2, 0, $magnet_adapter_magnet_base])
                cube([$spokes, $magnet_adapter_depth, $magnet_size[2]], center = false);
              // connector spokes for roof
              translate([-$spokes/2, 0, $magnet_adapter_magnet_base + $magnet_size[2]])
                cube([$spokes, $holder_inner_radius + $wall, $magnet_adapter_magnet_roof], center = false);
            }
          }
        }
      // clear out interior structures to vial diameter
      translate([0, 0, -$e])
        cylinder(d = inside_diameter, h = $magnet_adapter_height + $magnet_adapter_lip + $2e);
      }
    }
    // UNION END

    // DIFFERENCE START
    // magnet cutouts
    for (i = indices) {
      rotate([0, 0, -90 + $magnet_location[i]])
        translate([-$magnet_size[1]/2, $magnet_adapter_depth - $magnet_size[0], $magnet_adapter_magnet_base])
          cube([$magnet_size[1], $magnet_size[0] + $e, $magnet_size[2]], center = false);
    }

    // hollow ridge if not enough space for a full wall
    if ( ridges && ($holder_inner_diameter - inside_diameter - $nozzle) <= $wall) {
      for (i = indices)
        rotate([0, 0, -90 + $magnet_location[i]])
          translate([-$magnet_adapter_ridge_width/2 + (1.1 * $wall/2)/2, $holder_inner_radius - 1.6 * $wall, $magnet_adapter_height])
            cube([$magnet_adapter_ridge_width - 1.1 * $wall/2, $magnet_adapter_ridge_depth - $wall/4 - $holder_inner_radius + (1.6 - 0.05) * $wall, $magnet_adapter_lip + $e], center = false);
    }

    // sensors
    if (sensors)
      for (i = indices) {
        rotate([0, 0, -90 + $magnet_location[i]]) {
          // sensor holes
          translate([0, 0, $sensor_start])
            rotate([-90, 0, 0])
              cylinder(d = $sensor_cable_diameter, h = inside_diameter/2 + $sensor_thickness + $wall + $e);
          // top screws
          translate([0, inside_diameter/2 + $sensor_thickness - $sensor_setscrew_diameter/2, $sensor_start])
            cylinder(d = $sensor_setscrew_diameter, h = $sensor_roof + $e);
          // sensor spokes
          if (spokes) {
            translate([-$spokes/2, 0, $magnet_adapter_magnet_base + $magnet_size[2] + $magnet_adapter_magnet_roof])
              cube([$spokes, inside_diameter/2 + $sensor_thickness, $sensor_start + $sensor_roof - $magnet_adapter_magnet_base - $magnet_size[2] - $magnet_adapter_magnet_roof], center = false);
          }
        }
      }
    // DIFFERENCE END
  }
}

// crush ribs ring
module crush_ribs_ring(vial_diameter, height) {
  r = vial_diameter/2 + $flex_ribs_depth;
  union() {
    difference() {
      // adapter walls
      cylinder(h = height, d = 2 * r + 2 * $wall);
      // vial cutout
      translate([0, 0, -$e]) cylinder(h = height + $2e, d = 2 * r);
    }
    // flex ribs
    depth = ($flex_ribs_depth^2/2 - $flex_ribs_depth * r + r^2 - r * $flex_ribs_curvature * r) / (r - $flex_ribs_curvature * r);
    for (i = [1:1:$flex_ribs_n]) {
      rotate([0, 0, i * 360/$flex_ribs_n])
      translate([$flex_ribs_width, 0, 0])
      difference() {
        // rib outside
        translate([r - r * $flex_ribs_curvature, 0, 0])
          cylinder(r = r * $flex_ribs_curvature, h = height);
        // rib inside
        translate([r - r * $flex_ribs_curvature, 0, -$e])
          cylinder(r = r * $flex_ribs_curvature - $flex_ribs_width, h = height + $2e);
        // cut away extraneous part of the circle
        translate([-r, -r - $e, -$e])
          cube([r + depth - $flex_ribs_width, 2 * r + $2e, height + $2e], center = false);
        translate([-r, 0, -$e])
          cube([2 * r + $e, r + $e, height + $2e], center = false);
      }
    }
  }
}

/** bottle/vial adapters **/

// bottle adapter sizes
$adapter_gl45_100ml = 56.5 - 2.5; // 100mL media bottle (CONFIRMED)
$adapter_serum_60ml = 41 - 1.25; // 60ml serum bottle (CONFIRMED)
$adapter_serum_120ml = 51.5 - 2.0; // 120ml serium bottles (CONFIRMED)
$adapter_serum_160ml = 54.5 - 2.5; // 160ml serum bottle (CONFIRMED)
$adapter_culture_60ml = 25.4 - 1.0; // 60ml long culture bottle (CONFIRMED)
$adapter_balch = 18.0 - 0.5; // balch and culture tubes (CONFIRMED)

$sensor_thickness = 6; // thickness of sensor port
$sensor_start = 10; // height of sensor
$sensor_roof = 3; // roof/additional height above sensor location
$sensor_setscrew_diameter = 2.25; // for M2 screw
$sensor_cable_diameter = 3.0; // diameter of the cable

$flex_ribs_n = 8; // number of ribs to use
$flex_ribs_depth = 3; // perpendicular distance from ring wall to tip of ribs
$flex_ribs_curvature = 0.7; // as a fraction of the vial radius
$flex_ribs_width = $nozzle + 0.01; // thickness in mm (doesn't always slice correctly with only 0.5)

// module adapter
module bottle_adapter(vial_diameter, height = $magnet_adapter_height + $magnet_adapter_lip, spokes = true, sensors = false) {
  r = vial_diameter/2 + $flex_ribs_depth;
  with_adapter_magnets(inside_diameter = 2 * r, spokes = spokes, sensors = sensors)
  crush_ribs_ring(vial_diameter, height);
}

/** stirrer magnet holder **/

// stirrers
$stirrer_gear_height = 6.0; // height of the gear teeth
$stirrer_gear_diameter = 11.0 + 2 * $extra_tolerance; // diameter of the teeth
$stirrer_gear_wall = $wall/2; // thickness of the gear wall
$stirrer_gear_teeth_n = 18; // number of teeth
$stirrer_gear_teeth_radius = 0.5; // radius of the teeth attachment
$stirrer_magnet_diameter_small = 6.5 + 2 * $extra_tolerance; // 1/4
$stirrer_magnet_diameter_medium = 8.0 + 2 * $extra_tolerance; // 5/16
$stirrer_magnet_diameter_large = 9.52 + 2 * $extra_tolerance; // 3/8
$stirrer_magnet_height = 3.2; // height of the magnets
$stirrer_magnet_edge_distance = 7.0; // 5.0; // edge-to-edge distance of the magnets
$stirrer_magnet_base = $layer_height; // thickness of base underneath magnet
$stirrer_magnet_wall = $stirrer_gear_wall; // thickness of walls holding the magnets
$stirrer_cuts = 0.1; // print path cuts

// stirrer magnet holder
module stirrer_magnet_holder(stirrer_magnet_diameter) {
  union() {
    difference() {
      // base
      union() {
        // magnet circles
        for (x = [-1, 1])
          translate([x * (stirrer_magnet_diameter/2 + $stirrer_magnet_edge_distance/2), 0, 0])
            cylinder(d = stirrer_magnet_diameter + 2 * $stirrer_magnet_wall, h = $stirrer_magnet_height + $stirrer_magnet_base + $stirrer_gear_height);
        // bridge
        // translate([0, 0, ($stirrer_magnet_height + $stirrer_magnet_base)/2])
        //  cube([stirrer_magnet_diameter + $stirrer_magnet_edge_distance, stirrer_magnet_diameter + 2 * $stirrer_magnet_wall, $stirrer_magnet_height + $stirrer_magnet_base], true);
        // gear adapter
        cylinder(d = $stirrer_gear_diameter + 2 * $stirrer_gear_wall, h = $stirrer_magnet_height + $stirrer_magnet_base + $stirrer_gear_height);
      }
      // magnet cutous
      for (x = [-1, 1])
        translate([x * (stirrer_magnet_diameter/2 + $stirrer_magnet_edge_distance/2), 0, $stirrer_magnet_base])
          cylinder(d = stirrer_magnet_diameter, h = $stirrer_magnet_height + $stirrer_gear_height + $e);
      // bridge cutout
      //translate([0, 0, $stirrer_gear_height/2 + $e + $stirrer_magnet_height + $stirrer_magnet_base])
      //  cube([$stirrer_gear_diameter + 2 * $stirrer_gear_wall + $2e, stirrer_magnet_diameter, $stirrer_gear_height + $2e], true);
      // gear cutout
      translate([0, 0, $stirrer_magnet_base])
        difference() {
          cylinder(d = $stirrer_gear_diameter, h = $stirrer_magnet_height + $stirrer_gear_height + $e);
          for (i = [1:1:$stirrer_gear_teeth_n]) {
            translate([$stirrer_gear_diameter/2 * cos((i-1) * 360/$stirrer_gear_teeth_n), $stirrer_gear_diameter/2 * sin((i-1) * 360/$stirrer_gear_teeth_n), -$e])
              cylinder(r = $stirrer_gear_teeth_radius, h = $stirrer_magnet_height + $stirrer_gear_height + $e + $2e);
          }
        }
    }
    // base rings
    for (x = [-1, 1])
      translate([x * (stirrer_magnet_diameter/2 + $stirrer_magnet_edge_distance/2), 0, $stirrer_magnet_base])
        difference() {
          cylinder(d = stirrer_magnet_diameter + 2 * $stirrer_magnet_wall, h = $stirrer_magnet_height);
          cylinder(d = stirrer_magnet_diameter, h = $stirrer_magnet_height + $e);
          translate([-x * (stirrer_magnet_diameter + $stirrer_magnet_wall)/2, 0, $stirrer_magnet_height/2])
            cube([$stirrer_magnet_wall + $2e, $stirrer_cuts, $stirrer_magnet_height + $2e], center = true);
        }

  }
}

/** utility functions **/

// distribute cube along the x axis
// @param align: where to align relative to x axis - 0 (left aligned, starting at x=0), center = 0.5, 1 (right aligned, ending at x = 0)
// @param offset: offset in the y axis
// @param alternate: whether to alternate offset direction
module distribute_along_x(size, length, spacing = $wall/2, pad_left = $wall/2, pad_right = $wall/2, align = 0.5, offset = 0, alternate = false) {
  length_for_sizes = length - pad_left - pad_right;
  n_sizes = round(length_for_sizes / spacing);
  if (n_sizes > 0) {
    dx = length_for_sizes / n_sizes;
    for (i = [0:1:n_sizes])
      translate([-size[0]/2 - align * length + pad_left + i * dx, offset * ((alternate && i % 2 == 0) ? -1 : 1), 0])
        cube(size, center = false);
  }
}

// distribute cube along the y axis
// @param align: where to align relative to y axis - 0 (left aligned, starting at y=0), center = 0.5, 1 (right aligned, ending at y = 0)
// @param offset: offset in the x axis
// @param alternate: whether to alternate offset direction
module distribute_along_y(size, length, spacing = $wall/2, pad_left = $wall/2, pad_right = $wall/2, align = 0.5, offset = 0, alternate = false) {
  length_for_sizes = length - pad_left - pad_right;
  n_sizes = round(length_for_sizes / spacing);
  if (n_sizes > 0) {
    dy = length_for_sizes / n_sizes;
    for (i = [0:1:n_sizes])
      translate([offset * ((alternate && i % 2 == 0) ? -1 : 1), -size[1]/2 - align * length + pad_left + i * dy, 0])
        cube(size, center = false);
  }
}

/** light adapter **/

$light_adapter_fan_size = 32;
$light_adapter_fan_diameter = 25;
$light_adapter_fan_screw_depth = 2;
$light_adapter_fan_screw_location = 12; // screw locations
$light_adapter_fan_screw_diameter = get_screw("M3")[1] - 0.25;; // screw diameter
$light_adapter_up_diameter = $holder_inner_diameter + 10.5; // account for the light sheet width
$light_adapter_up_height = $light_adapter_fan_size;
$light_adapter_down_diameter = $holder_inner_diameter - 2 * $magnet_adapter_min_gap;
$light_adapter_down_height = $magnet_adapter_height + $holder_base_height + $holder_sensors_height - 2; // down height with a little offset
$light_adapter_gap = 0.25;
$light_adapter_sheet = 2; // gap for wedging light sheet in
$light_adapter_catch = 15; // angle for wedge cutout

module light_adapter_up(height = $light_adapter_up_height, spokes = true) {
  fan_depth = $light_adapter_up_diameter/2 + $light_adapter_fan_screw_depth;
  difference() {
      // cylinder
      with_fan(height, fan_depth, spokes = spokes)
        with_adapter_magnets(base_insert = false, ridges = false)
          cylinder(d = $light_adapter_up_diameter, h = height);
      // core cutout
      translate([0, 0, -$e])
        cylinder(d = $light_adapter_up_diameter - 2 * $wall, h = height + $2e);
  }
}

module light_adapter_down(spokes = true) {
  fan_depth = $holder_inner_radius + $wall + $light_adapter_fan_screw_depth;
  difference() {
    // cylinder
    with_wedge_cutout(dangle = 7, zoffset = $light_adapter_up_height)
      with_fan($light_adapter_up_height, fan_depth, spokes = spokes)
        with_base_magnets($light_adapter_up_height, spokes = spokes, ridges = false, last_layer = true)
          cylinder(d = $light_adapter_down_diameter, h = $light_adapter_down_height + $light_adapter_up_height);
    // core cutout
    translate([0, 0, -$e])
      cylinder(d = $light_adapter_down_diameter - 2 * $wall, h = $light_adapter_down_height + $light_adapter_up_height + $2e);
  }
}

module with_fan(height, fan_depth, spokes = true) {
  difference() {
    union() {
      // light adapter core
      children(0);
      // fan
      difference() {
        // fan adapter
        rotate([0, 0, 90])
          translate([0, -$light_adapter_fan_size/2, 0])
            cube([fan_depth, $light_adapter_fan_size, height], center = false);
        // fan attachment screws
        rotate([0, 0, 90])
          for (y = [-1, 1])
            for (z = [-1, 1])
              translate([0, y * $light_adapter_fan_screw_location, height - $light_adapter_fan_size/2 + z * $light_adapter_fan_screw_location])
                resize([0, 0, $light_adapter_fan_screw_diameter + 2 * $screw_threadable_tolerance + 2 * $screw_vertical_stretch])
                  rotate([0, 90, 0])
                    cylinder(d = $light_adapter_fan_screw_diameter + 2 * $screw_threadable_tolerance, h = $holder_inner_diameter + 4 * $wall + $e);
        // spokes
        if (spokes) {
          rotate([0, 0, 90])
            for (y = [-1, 1])
              translate([0, y * $light_adapter_fan_screw_location, -$e])
                cube([$holder_inner_diameter + 4 * $wall + $e, $spokes, height + $2e]);
        }
      }
    }
    // fan center cutout
    rotate([0, 0, 90])
      translate([0, 0, height - $light_adapter_fan_size/2])
        rotate([0, 90, 0])
          cylinder(d = $light_adapter_fan_diameter, h = fan_depth + $e);
    // fan top cutout
    rotate([0, 0, 90])
      translate([0, -$light_adapter_fan_screw_location + $light_adapter_fan_screw_diameter, height - $light_adapter_fan_size/2])
        cube([fan_depth + $e, 2 * $light_adapter_fan_screw_location - 2 * $light_adapter_fan_screw_diameter, height/2 + $e], center = false);
    // spokes
    if (spokes)
      rotate([0, 0, 90])
        translate([0, 0, -$e])
          cube([fan_depth + $e, $spokes, height + $2e]);
  }
}

module with_wedge_cutout(dangle = 0, zoffset = -$e) {
  echo($magnet_location[2] - $magnet_location[1] + 2 * dangle);
  difference() {
    children(0);
    r = 2 * $magnet_adapter_depth;
    translate([0, 0, zoffset])
      difference() {
        cylinder(d = 2 * r, h = $light_adapter_down_height + $2e);
        for (x = [0, 1])
          rotate([0, 0, x * 180])
            translate([0, 0, -$e])
              linear_extrude($light_adapter_down_height + 3 * $e)
                polygon([
                  [0, 0],
                  [r * cos($magnet_location[1] - dangle), r * sin($magnet_location[1] - dangle)],
                  [r * cos($magnet_location[2] + dangle), r * sin($magnet_location[2] + dangle)],
                ]);
      }
  }
}

/** bottle base **/

$bottle_base_thickness = 4.0; // thickness of bottle adapter base
$bottle_base_expand = 1.15; // expanding for tight fit
$bottle_base_gap = 2; // gap to edge (in mm)
$bottle_clip_position = 0.8 * $holder_inner_diameter; // position of clip
$bottle_clip_stretch = 5; // y stretch of clip
$bottle_base_center_diameter = 16; // the hole in the center
$bottle_base_outer_diameter = 28; // the outside diameter
$bottle_base_notch_gap = $nozzle;

// make the botte base adapter
module bottle_base(spokes = true, inner_diameter = $bottle_base_center_diameter, outer_diameter = $bottle_base_outer_diameter + 2 * $wall, height = $bottle_base_thickness, notch_thickness = 2 * $holder_bottom_thickness) {
  connector = $holder_notch_position + $holder_notch_length/2 - outer_diameter/2 + $wall/2 - $bottle_base_notch_gap/2;
  difference() {
    union() {
      // center
      difference() {
        union() {
          cylinder(d = outer_diameter, h = height);
          // connectors
          for (a = [0, 180])
            rotate([0, 0, a])
              translate([-($holder_notch_width - $bottle_base_notch_gap)/2, -connector + $holder_notch_position + $holder_notch_length/2 - $bottle_base_notch_gap/2, 0])
                cube([$holder_notch_width - $bottle_base_notch_gap, connector, height]);
        }
        translate([0, 0, -$e])
          cylinder(d = inner_diameter , h = height + $2e);
        // spokes
        if (spokes) {
          rotate([0, 0, 90])
            translate([0, 0, -$e])
              cube([$spokes, outer_diameter/2 + $e, height + $2e]);
        }
      }
      // notches
      for (a = [0, 180])
        rotate([0, 0, a])
          translate([-($holder_notch_width - $bottle_base_notch_gap)/2, -$holder_notch_length/2 + $bottle_base_notch_gap/2 - $holder_notch_position, -notch_thickness])
            cube([$holder_notch_width - $bottle_base_notch_gap, $holder_notch_length - 3 * $bottle_base_notch_gap, notch_thickness]);
    }
    // spokes
    if (spokes) {
      for (y = [-1, 1])
        translate([-$spokes/2, -(connector - $wall/2)/2 + y * ($holder_notch_position + $holder_notch_length/2 - $bottle_base_notch_gap/2 - connector/2 + $wall/4), 0])
          cube([$spokes, connector - $wall/2, height + $e]);
      for (a = [0, 180])
        rotate([0, 0, a])
          translate([-$spokes/2, outer_diameter/2, -notch_thickness - $e])
            cube([$spokes, connector - $wall, notch_thickness + $e]);
    }

  }
}

/** prints **/

//device();
//stirrer_magnet_holder($stirrer_magnet_diameter_medium);
//bottle_base();
//bottle_adapter($adapter_balch);
//bottle_adapter($adapter_gl45_100ml);
//bottle_adapter($adapter_serum_160ml);
//bottle_adapter($adapter_serum_120ml);
//bottle_adapter($adapter_serum_60ml);
//bottle_adapter($adapter_culture_60ml);
//light_adapter_down();
//light_adapter_up();

// speciality parts - 60mL culture tube base anchor for motor free use (i.e. tube supported only)
union() {
  bottle_base(inner_diameter = $adapter_culture_60ml + 2 * $wall + 2 * $bottle_base_gap, outer_diameter = $adapter_culture_60ml + 2 * $wall + 2 * $bottle_base_gap, height = $holder_base_height );
  crush_ribs_ring(vial_diameter = $adapter_culture_60ml, height = $holder_base_height);
}

