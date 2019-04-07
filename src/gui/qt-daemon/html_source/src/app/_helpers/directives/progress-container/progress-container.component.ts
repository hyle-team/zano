import { Component, Input, OnInit } from '@angular/core';

@Component({
  selector: 'app-progress-container',
  templateUrl: './progress-container.component.html',
  styleUrls: ['./progress-container.component.scss']
})
export class ProgressContainerComponent implements OnInit {

  @Input() width: string;
  @Input() labels: [];

  constructor() {}

  ngOnInit() {}

}
