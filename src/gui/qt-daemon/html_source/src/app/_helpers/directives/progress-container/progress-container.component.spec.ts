import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { ProgressContainerComponent } from './progress-container.component';

describe('ProgressContainerComponent', () => {
  let component: ProgressContainerComponent;
  let fixture: ComponentFixture<ProgressContainerComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ ProgressContainerComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(ProgressContainerComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
