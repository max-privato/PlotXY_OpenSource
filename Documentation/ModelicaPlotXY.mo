package ModelicaPlotXY
  model ChopperStepDown "Step down chopper with resistive load"
    extends Modelica.Electrical.PowerConverters.Examples.DCDC.ExampleTemplates.ChopperStepDown;
    extends Modelica.Icons.Example;
    parameter Modelica.SIunits.Resistance R = 100 "Resistance";
    Modelica.Electrical.Analog.Basic.Resistor resistor(R = 30) annotation(
      Placement(visible = true, transformation(origin = {38, 22}, extent = {{-10, -10}, {10, 10}}, rotation = 270)));
  Modelica.Electrical.Analog.Basic.Resistor Rf(R = 0.5) annotation(
      Placement(visible = true, transformation(origin = {-10, 50}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
  Modelica.Electrical.Analog.Basic.Inductor Lf(L = 0.03)  annotation(
      Placement(visible = true, transformation(origin = {18, 50}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
  Modelica.Electrical.Analog.Basic.Capacitor Df(C = 100e-6)  annotation(
      Placement(visible = true, transformation(origin = {16, 22}, extent = {{-10, -10}, {10, 10}}, rotation = -90)));
  equation
    connect(resistor.n, currentSensor.p) annotation(
      Line(points = {{38, 12}, {38, -6}, {0, -6}}, color = {0, 0, 255}));
  connect(Rf.p, chopperStepDown.dc_p2) annotation(
      Line(points = {{-20, 50}, {-30, 50}, {-30, 6}, {-40, 6}}, color = {0, 0, 255}));
  connect(Rf.n, Lf.p) annotation(
      Line(points = {{0, 50}, {8, 50}}, color = {0, 0, 255}));
  connect(Lf.n, resistor.p) annotation(
      Line(points = {{28, 50}, {38, 50}, {38, 32}}, color = {0, 0, 255}));
  connect(Df.p, resistor.p) annotation(
      Line(points = {{16, 32}, {38, 32}}, color = {0, 0, 255}));
  connect(Df.n, resistor.n) annotation(
      Line(points = {{16, 12}, {38, 12}}, color = {0, 0, 255}));
    annotation(
      experiment(StartTime = 0, StopTime = 0.1, Tolerance = 1e-06, Interval = 0.0002),
      Documentation(info = "<html>
<p>This example demonstrates the switching on of a resistive load operated by a step down chopper.
DC output voltage is equal to <code>dutyCycle</code> times the input voltage.
Plot current <code>currentSensor.i</code>, averaged current <code>meanCurrent.y</code>, total voltage <code>voltageSensor.v</code> and voltage <code>meanVoltage.v</code>.</p>
</html>"),
  Diagram(coordinateSystem(extent = {{-100, 80}, {100, -60}})));
  end ChopperStepDown;
  
  model MultiPhaseTwoLevel_R "Multi phase DC to AC converter with R load"
    extends Modelica.Icons.Example;
    parameter Integer m = 3 "Number of phases";
    parameter Modelica.SIunits.Frequency f = 1000 "Switching frequency";
    parameter Modelica.SIunits.Frequency f1 = 50 "Fundamental wave AC frequency";
    parameter Modelica.SIunits.Resistance R = 100 "Resistance";
    Modelica.Electrical.Analog.Sources.ConstantVoltage constantVoltage_n(V = 50) annotation(
      Placement(visible = true, transformation(origin = {-70, 2}, extent = {{-10, -10}, {10, 10}}, rotation = 270)));
    Modelica.Electrical.PowerConverters.DCAC.MultiPhase2Level inverter(useHeatPort = false, m = m) annotation(
      Placement(visible = true, transformation(extent = {{-50, 12}, {-30, 32}}, rotation = 0)));
    Modelica.Electrical.Analog.Basic.Ground ground annotation(
      Placement(visible = true, transformation(origin = {-90, 16}, extent = {{-10, 10}, {10, -10}}, rotation = 180)));
    Modelica.Electrical.PowerConverters.DCDC.Control.SignalPWM signalPWM[m](each useConstantDutyCycle = false, each f = f) annotation(
      Placement(visible = true, transformation(origin = {-40, -26}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
    Modelica.Electrical.Analog.Sources.ConstantVoltage constantVoltage_p(V = 50) annotation(
      Placement(visible = true, transformation(origin = {-70, 42}, extent = {{-10, -10}, {10, 10}}, rotation = 270)));
    Modelica.Blocks.Sources.Sine sine[m](phase = -Modelica.Electrical.MultiPhase.Functions.symmetricOrientation(m), startTime = zeros(m), amplitude = fill(0.5, m), offset = fill(0.5, m), freqHz = fill(f1, m)) annotation(
      Placement(visible = true, transformation(origin = {-82, -26}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
    Modelica.Electrical.MultiPhase.Basic.Resistor resistor( R = fill(10, 3),m = m) annotation(
      Placement(visible = true, transformation(origin = {44, 2}, extent = {{-10, -10}, {10, 10}}, rotation = 270)));
    Modelica.Electrical.MultiPhase.Basic.Star star(m = m) annotation(
      Placement(visible = true, transformation(origin = {44, -28}, extent = {{-10, -10}, {10, 10}}, rotation = 270)));
  Modelica.Electrical.MultiPhase.Basic.Capacitor Cf(C = fill(0.000634, 3)) annotation(
      Placement(visible = true, transformation(origin = {26, 0}, extent = {{-10, -10}, {10, 10}}, rotation = -90)));
  Modelica.Electrical.MultiPhase.Basic.Inductor Lf(L = fill(0.001, 3)) annotation(
      Placement(visible = true, transformation(origin = {16, 22}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
  Modelica.Electrical.MultiPhase.Basic.Resistor Rf(R = fill(0.05, 3)) annotation(
      Placement(visible = true, transformation(origin = {-8, 22}, extent = {{-10, -10}, {10, 10}}, rotation = 0)));
  equation
    connect(constantVoltage_p.n, constantVoltage_n.p) annotation(
      Line(points = {{-70, 32}, {-70, 12}}, color = {0, 0, 255}));
    connect(constantVoltage_p.p, inverter.dc_p) annotation(
      Line(points = {{-70, 52}, {-60, 52}, {-60, 28}, {-50, 28}}, color = {0, 0, 255}));
    connect(constantVoltage_n.n, inverter.dc_n) annotation(
      Line(points = {{-70, -8}, {-60, -8}, {-60, 16}, {-54, 16}, {-50, 16}}, color = {0, 0, 255}));
    connect(ground.p, constantVoltage_p.n) annotation(
      Line(points = {{-90, 26}, {-70, 26}, {-70, 32}}, color = {0, 0, 255}));
    connect(sine.y, signalPWM.dutyCycle) annotation(
      Line(points = {{-71, -26}, {-52, -26}}, color = {0, 0, 127}));
    connect(signalPWM.fire, inverter.fire_p) annotation(
      Line(points = {{-46, -15}, {-46, 10}}, color = {255, 0, 255}));
    connect(signalPWM.notFire, inverter.fire_n) annotation(
      Line(points = {{-34, -15}, {-34, 10}}, color = {255, 0, 255}));
    connect(inverter.ac, Rf.plug_p) annotation(
      Line(points = {{-30, 22}, {-18, 22}}, color = {0, 0, 255}));
    connect(Rf.plug_n, Lf.plug_p) annotation(
      Line(points = {{2, 22}, {6, 22}}, color = {0, 0, 255}));
    connect(Lf.plug_n, resistor.plug_p) annotation(
      Line(points = {{26, 22}, {44, 22}, {44, 12}}, color = {0, 0, 255}));
    connect(Lf.plug_n, Cf.plug_p) annotation(
      Line(points = {{26, 22}, {26, 10}}, color = {0, 0, 255}));
    connect(Cf.plug_n, resistor.plug_n) annotation(
      Line(points = {{26, -10}, {44, -10}, {44, -8}}, color = {0, 0, 255}));
  connect(star.plug_p, resistor.plug_n) annotation(
      Line(points = {{44, -18}, {44, -8}}, color = {0, 0, 255}));
    annotation(
      experiment(StartTime = 0, StopTime = 0.1, Tolerance = 1e-06, Interval = 0.00002),
      Documentation(info = "<html>
  <p>Plot current <code>currentSensor.i[:]</code>, harmonic current magnitude <code>fundamentalWaveCurrent[:].y_RMS</code>, harmonic voltage magnitude <code>fundamentalWaveVoltage[:].y_RMS</code>. The instantaneous voltages <code>voltageSensor.i[:]</code> and currents <code>currentSensor.i[:]</code> directly show the switching pattern of the inverter. There is not smoothing effect due to an inductance in this example; see <a href=\"modelica://Modelica.Electrical.PowerConverters.Examples.DCAC.MultiPhaseTwoLevel.MultiPhaseTwoLevel_RL\">MultiPhaseTwoLevel_RL</a>.</p>
  </html>"),
      Diagram(coordinateSystem(extent = {{-100, 60}, {60, -40}})));
  end MultiPhaseTwoLevel_R;
  annotation(
    uses(Modelica(version = "3.2.3")));
end ModelicaPlotXY;
