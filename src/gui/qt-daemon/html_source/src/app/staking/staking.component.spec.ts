import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { StakingComponent } from './staking.component';

describe('StakingComponent', () => {
  let component: StakingComponent;
  let fixture: ComponentFixture<StakingComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ StakingComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(StakingComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
